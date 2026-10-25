#include "ble_measurements.h"
#include "device_identity.h"
#include "measurement_snapshot.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
extern "C"
{
#include "nimble/nimble_port.h"
#include "host/ble_hs.h"
#include "host/ble_att.h"
#include "host/ble_gatt.h"
#include "host/ble_uuid.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
}
#include <atomic>
#include <cstring>

namespace
{
constexpr const char *tag = "aqm_ble";
constexpr const char *device_name = "Air Quality Monitor";
// NimBLE stores 128-bit UUIDs in least-significant-byte-first order.
const ble_uuid128_t service_uuid = BLE_UUID128_INIT(
    0x01,0xec,0x05,0x82,0x0b,0x98,0x39,0xb3,0xc0,0x4b,0x6f,0x0a,0x3f,0xc6,0x98,0x35);
const ble_uuid128_t measurement_uuid = BLE_UUID128_INIT(
    0x8c,0x43,0x03,0x41,0x15,0x02,0x45,0xb8,0x22,0x41,0x57,0x56,0xc5,0x39,0xaa,0x46);
const ble_uuid16_t dis_uuid = BLE_UUID16_INIT(0x180a);
const ble_uuid16_t serial_uuid = BLE_UUID16_INIT(0x2a25);
char serial[37];
uint16_t measurement_handle;
uint16_t connection = BLE_HS_CONN_HANDLE_NONE;
uint16_t mtu = 23;
uint8_t own_address_type;
bool synchronized = false, subscribed = false;
std::atomic<bool> running{false};
portMUX_TYPE snapshot_lock = portMUX_INITIALIZER_UNLOCKED;
aqm::MeasurementSnapshot snapshot;
uint64_t generation = 0, subscription_floor = 0, attempted = 0;
ble_npl_callout publication_poll;
ble_npl_callout advertising_retry;
int64_t last_warning = -5000000;

void warn(const char *operation, int error)
{
    const int64_t now = esp_timer_get_time();
    if (now - last_warning >= 5000000)
    {
        ESP_LOGW(tag, "%s failed (%d); acquisition continues", operation, error);
        last_warning = now;
    }
}

aqm::EncodedMeasurement read_snapshot(uint64_t *version = nullptr)
{
    portENTER_CRITICAL(&snapshot_lock);
    auto value = snapshot.read(esp_timer_get_time());
    if (version) *version = generation;
    portEXIT_CRITICAL(&snapshot_lock);
    return value;
}

int access(uint16_t, uint16_t, ble_gatt_access_ctxt *context, void *arg)
{
    if (context->op != BLE_GATT_ACCESS_OP_READ_CHR) return BLE_ATT_ERR_READ_NOT_PERMITTED;
    // NimBLE ble_gatts_val_access slices the full value for Read Blob offsets.
    if (arg == serial)
    {
        return os_mbuf_append(context->om, serial, 36) == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }
    const auto value = read_snapshot();
    return os_mbuf_append(context->om, value.bytes, value.size) == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
}

ble_gatt_chr_def dis_characteristics[2] = {};
ble_gatt_chr_def measurement_characteristics[2] = {};
ble_gatt_svc_def services[3] = {};

void define_services()
{
    dis_characteristics[0].uuid = &serial_uuid.u;
    dis_characteristics[0].access_cb = access;
    dis_characteristics[0].arg = serial;
    dis_characteristics[0].flags = BLE_GATT_CHR_F_READ;
    measurement_characteristics[0].uuid = &measurement_uuid.u;
    measurement_characteristics[0].access_cb = access;
    measurement_characteristics[0].flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY;
    measurement_characteristics[0].val_handle = &measurement_handle;
    services[0].type = BLE_GATT_SVC_TYPE_PRIMARY;
    services[0].uuid = &dis_uuid.u;
    services[0].characteristics = dis_characteristics;
    services[1].type = BLE_GATT_SVC_TYPE_PRIMARY;
    services[1].uuid = &service_uuid.u;
    services[1].characteristics = measurement_characteristics;
}

void notify(ble_npl_event *)
{
    if (!synchronized || connection == BLE_HS_CONN_HANDLE_NONE || !subscribed) return;
    uint64_t version;
    const auto value = read_snapshot(&version);
    if (version <= subscription_floor || version <= attempted) return;
    attempted = version;
    if (!aqm::notification_fits(value.size, mtu))
    {
        warn("notification exceeds negotiated MTU", mtu);
        return;
    }
    os_mbuf *buffer = ble_hs_mbuf_from_flat(value.bytes, value.size);
    if (!buffer)
    {
        warn("notification allocation", BLE_HS_ENOMEM);
        return;
    }
    // NimBLE consumes the mbuf on both success and failure, including empty values.
    int error = ble_gatts_notify_custom(connection, measurement_handle, buffer);
    if (error) warn("notification", error);
}

void poll(ble_npl_event *)
{
    notify(nullptr);
    if (synchronized)
    {
        ble_npl_callout_reset(&publication_poll, ble_npl_time_ms_to_ticks32(100));
    }
}

int gap_event(ble_gap_event *event, void *);
void advertise(ble_npl_event *)
{
    if (!synchronized || connection != BLE_HS_CONN_HANDLE_NONE || ble_gap_adv_active()) return;
    ble_hs_adv_fields fields = {};
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    fields.uuids128 = &service_uuid;
    fields.num_uuids128 = 1;
    fields.uuids128_is_complete = 1;
    int error = ble_gap_adv_set_fields(&fields);
    if (!error)
    {
        ble_hs_adv_fields response = {};
        response.name = reinterpret_cast<const uint8_t *>(device_name);
        response.name_len = std::strlen(device_name);
        response.name_is_complete = 1;
        error = ble_gap_adv_rsp_set_fields(&response);
    }
    if (!error)
    {
        ble_gap_adv_params params = {};
        params.conn_mode = BLE_GAP_CONN_MODE_UND;
        params.disc_mode = BLE_GAP_DISC_MODE_GEN;
        error = ble_gap_adv_start(own_address_type, nullptr, BLE_HS_FOREVER, &params, gap_event, nullptr);
    }
    if (error)
    {
        warn("advertising", error);
        ble_npl_callout_reset(&advertising_retry, ble_npl_time_ms_to_ticks32(1000));
    }
}

void clear_connection()
{
    connection = BLE_HS_CONN_HANDLE_NONE;
    subscribed = false;
    mtu = 23;
}
int gap_event(ble_gap_event *event, void *)
{
    switch (event->type)
    {
    case BLE_GAP_EVENT_CONNECT:
        if (event->connect.status == 0)
        {
            connection = event->connect.conn_handle;
            subscribed = false;
            mtu = 23;
        }
        else advertise(nullptr);
        break;
    case BLE_GAP_EVENT_DISCONNECT:
        clear_connection();
        advertise(nullptr);
        break;
    case BLE_GAP_EVENT_SUBSCRIBE:
        if (event->subscribe.conn_handle == connection && event->subscribe.attr_handle == measurement_handle)
        {
            subscribed = event->subscribe.cur_notify;
            portENTER_CRITICAL(&snapshot_lock);
            subscription_floor = generation;
            portEXIT_CRITICAL(&snapshot_lock);
        }
        break;
    case BLE_GAP_EVENT_MTU:
        if (event->mtu.conn_handle == connection) mtu = event->mtu.value >= 23 ? event->mtu.value : 23;
        break;
    case BLE_GAP_EVENT_ADV_COMPLETE:
        advertise(nullptr);
        break;
    default:
        break;
    }
    return 0;
}
void on_reset(int reason)
{
    synchronized = false;
    clear_connection();
    ble_npl_callout_stop(&advertising_retry);
    ble_npl_callout_stop(&publication_poll);
    warn("host reset", reason);
}
void on_sync()
{
    clear_connection();
    int error = ble_hs_util_ensure_addr(0);
    if (!error) error = ble_hs_id_infer_auto(0, &own_address_type);
    if (error)
    {
        warn("BLE address", error);
        return;
    }
    synchronized = true;
    ble_npl_callout_reset(&publication_poll, ble_npl_time_ms_to_ticks32(100));
    advertise(nullptr);
}
void host_task(void *)
{
    nimble_port_run();
    running.store(false);
    vTaskDelete(nullptr);
}
}

namespace aqm
{
esp_err_t start_ble()
{
    esp_err_t error = load_device_identity(serial);
    if (error != ESP_OK)
    {
        ESP_LOGE(tag, "Identity storage failed (%s); BLE disabled for this boot; storage preserved", esp_err_to_name(error));
        return error;
    }
    error = nimble_port_init();
    if (error != ESP_OK) return error;
    ble_svc_gap_init();
    ble_svc_gatt_init();
    define_services();
    int rc = ble_svc_gap_device_name_set(device_name);
    if (!rc) rc = ble_att_set_preferred_mtu(80);
    if (!rc) rc = ble_gatts_count_cfg(services);
    if (!rc) rc = ble_gatts_add_svcs(services);
    if (rc)
    {
        nimble_port_deinit();
        return ESP_FAIL;
    }
    ble_hs_cfg.reset_cb = on_reset;
    ble_hs_cfg.sync_cb = on_sync;
    ble_hs_cfg.sm_bonding = 0;
    ble_hs_cfg.sm_mitm = 0;
    rc = ble_npl_callout_init(&publication_poll, nimble_port_get_dflt_eventq(), poll, nullptr);
    if (rc)
    {
        nimble_port_deinit();
        return ESP_ERR_NO_MEM;
    }
    rc = ble_npl_callout_init(&advertising_retry, nimble_port_get_dflt_eventq(), advertise, nullptr);
    if (rc)
    {
        ble_npl_callout_deinit(&publication_poll);
        nimble_port_deinit();
        return ESP_ERR_NO_MEM;
    }
    running.store(true);
    if (xTaskCreate(host_task, "aqm_ble", 4096, nullptr, 5, nullptr) != pdPASS)
    {
        running.store(false);
        ble_npl_callout_deinit(&advertising_retry);
        ble_npl_callout_deinit(&publication_poll);
        nimble_port_deinit();
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}
void publish_measurement(const Measurement &sample)
{
    if (!running.load()) return;
    EncodedMeasurement value;
    if (!encode_measurement(sample, value)) ESP_LOGW(tag, "Encoding failed; publish unavailable snapshot");
    portENTER_CRITICAL(&snapshot_lock);
    snapshot.replace(value, esp_timer_get_time());
    ++generation;
    portEXIT_CRITICAL(&snapshot_lock);
    // Host timer observes generation changes. Never enqueue here: the SDK
    // eventq_put can wait indefinitely for space in the host event queue.
}
}
