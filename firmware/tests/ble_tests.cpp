// Compile the production BLE implementation against fake SDK boundaries.
#include "../components/ble_measurements/ble_measurements.cpp"
#include <cassert>
#include <cstdio>
#include <cstdarg>
namespace
{
int64_t clock_us;
int identity_error, init_error, registration_error, callout_error, task_result = pdPASS;
int starts, sends, allocations, deinits, logs, mtu_preference;
bool advertising, fail_adv, fail_alloc, fail_append, fail_send;
os_mbuf notification;
ble_npl_eventq queue;
void pump()
{
    if (publication_poll.active) publication_poll.event.fn(nullptr);
}
void connect_client(uint16_t handle)
{
    advertising = false;
    ble_gap_event event = {}; event.type = BLE_GAP_EVENT_CONNECT;
    event.connect.conn_handle = handle; gap_event(&event, nullptr);
}
void subscribe_client(bool enabled)
{
    ble_gap_event event = {}; event.type = BLE_GAP_EVENT_SUBSCRIBE;
    event.subscribe.conn_handle = connection; event.subscribe.attr_handle = measurement_handle;
    event.subscribe.cur_notify = enabled; gap_event(&event, nullptr);
}
void disconnect_client()
{
    ble_gap_event event = {}; event.type = BLE_GAP_EVENT_DISCONNECT; gap_event(&event, nullptr);
}
}
namespace aqm
{
esp_err_t load_device_identity(char (&out)[37])
{
    if (identity_error) return identity_error;
    std::strcpy(out, "00010203-0405-4607-8809-0a0b0c0d0e0f"); return ESP_OK;
}
}
int64_t esp_timer_get_time() { return clock_us; }
const char *esp_err_to_name(int) { return "fake error"; }
void fake_log(const char *, const char *, ...) { ++logs; }
void vTaskDelete(void *) {}
int xTaskCreate(void (*)(void *), const char *, unsigned, void *, unsigned, void *) { return task_result; }
extern "C"
{
fake_hs_cfg ble_hs_cfg = {};
int nimble_port_init() { return init_error; }
int nimble_port_deinit() { ++deinits; return 0; }
void nimble_port_run() {}
ble_npl_eventq *nimble_port_get_dflt_eventq() { return &queue; }
int os_mbuf_append(os_mbuf *buffer, const void *data, size_t size)
{
    if (fail_append || buffer->size + size > sizeof(buffer->bytes)) return -1;
    std::memcpy(buffer->bytes + buffer->size, data, size); buffer->size += size; return 0;
}
os_mbuf *ble_hs_mbuf_from_flat(const void *data, size_t size)
{
    ++allocations; if (fail_alloc) return nullptr;
    notification = {}; os_mbuf_append(&notification, data, size); return &notification;
}
int ble_gatts_notify_custom(uint16_t handle, uint16_t attr, os_mbuf *buffer)
{
    assert(handle == connection && attr == measurement_handle && buffer == &notification);
    ++sends; return fail_send ? -1 : 0;
}
int ble_gap_adv_active() { return advertising; }
int ble_gap_adv_set_fields(const ble_hs_adv_fields *fields)
{
    assert(fields->num_uuids128 == 1 && fields->uuids128_is_complete);
    assert(!std::memcmp(fields->uuids128->value, service_uuid.value, 16));
    return fail_adv ? -1 : 0;
}
int ble_gap_adv_rsp_set_fields(const ble_hs_adv_fields *fields)
{
    assert(fields->name_len == 19 && fields->name_is_complete); return 0;
}
int ble_gap_adv_start(uint8_t, const void *, int, const ble_gap_adv_params *, int (*)(ble_gap_event *, void *), void *)
{
    advertising = true; ++starts; return 0;
}
int ble_hs_util_ensure_addr(int) { return 0; }
int ble_hs_id_infer_auto(int, uint8_t *address) { *address = 0; return 0; }
void ble_svc_gap_init() {}
void ble_svc_gatt_init() {}
int ble_svc_gap_device_name_set(const char *name) { assert(!std::strcmp(name, "Air Quality Monitor")); return 0; }
int ble_att_set_preferred_mtu(uint16_t value) { mtu_preference = value; return 0; }
int ble_gatts_count_cfg(const ble_gatt_svc_def *defs)
{
    assert(defs[0].type == BLE_GATT_SVC_TYPE_PRIMARY && defs[2].type == 0);
    assert(reinterpret_cast<const ble_uuid16_t *>(defs[0].uuid)->value == 0x180a);
    auto *serial_def = &defs[0].characteristics[0];
    assert(reinterpret_cast<const ble_uuid16_t *>(serial_def->uuid)->value == 0x2a25);
    assert(serial_def->flags == BLE_GATT_CHR_F_READ && !defs[0].characteristics[1].uuid);
    auto *reading_def = &defs[1].characteristics[0];
    assert(reading_def->flags == (BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY));
    assert(!defs[1].characteristics[1].uuid);
    return registration_error;
}
int ble_gatts_add_svcs(const ble_gatt_svc_def *) { measurement_handle = 42; return 0; }
void ble_npl_event_init(ble_npl_event *e, void (*fn)(ble_npl_event *), void *) { e->fn = fn; e->queued = false; }
void ble_npl_event_deinit(ble_npl_event *) {}
int ble_npl_callout_init(ble_npl_callout *c, ble_npl_eventq *, void (*fn)(ble_npl_event *), void *)
{
    c->event.fn = fn; return callout_error;
}
void ble_npl_callout_deinit(ble_npl_callout *) {}
int ble_npl_callout_reset(ble_npl_callout *c, uint32_t delay) { assert(delay == 1000 || delay == 100); c->active = true; return 0; }
void ble_npl_callout_stop(ble_npl_callout *c) { c->active = false; }
uint32_t ble_npl_time_ms_to_ticks32(uint32_t ms) { return ms; }
void ble_npl_eventq_put(ble_npl_eventq *, ble_npl_event *) { assert(false && "Acquisition must never enqueue to the blocking SDK event queue"); }
}
int main()
{
    identity_error = ESP_FAIL; assert(aqm::start_ble() != ESP_OK && !running.load());
    identity_error = 0; init_error = ESP_FAIL; assert(aqm::start_ble() != ESP_OK);
    init_error = 0; registration_error = -1; assert(aqm::start_ble() != ESP_OK && deinits == 1);
    registration_error = 0; callout_error = -1; assert(aqm::start_ble() != ESP_OK && deinits == 2);
    callout_error = 0; task_result = 0; assert(aqm::start_ble() != ESP_OK && deinits == 3 && !running.load());
    task_result = pdPASS; assert(aqm::start_ble() == ESP_OK && mtu_preference == 80);
    on_sync(); assert(advertising && starts == 1);
    os_mbuf read = {}; ble_gatt_access_ctxt context = {}; context.om = &read;
    assert(access(0, 0, &context, serial) == 0 && read.size == 36);
    context.offset = 22; read = {};
    assert(access(0, 0, &context, serial) == 0 && read.size == 36); // stack slices offsets
    fail_append = true; assert(access(0, 0, &context, serial) == BLE_ATT_ERR_INSUFFICIENT_RES); fail_append = false;
    context.op = 1; assert(access(0, 0, &context, nullptr) == BLE_ATT_ERR_READ_NOT_PERMITTED); context.op = 0;
    read = {}; assert(access(0, 0, &context, nullptr) == 0 && read.size == 0);
    advertising = false;
    ble_gap_event failed = {}; failed.type = BLE_GAP_EVENT_CONNECT; failed.connect.status = 1;
    gap_event(&failed, nullptr); assert(advertising && connection == BLE_HS_CONN_HANDLE_NONE);
    connect_client(7); subscribe_client(true);
    aqm::Measurement sample; sample.temperature_valid = true; sample.temperature = 0;
    aqm::publish_measurement(sample); pump(); assert(sends == 1 && notification.size == 5);
    aqm::publish_measurement(sample); pump(); assert(sends == 2); // unchanged
    subscribe_client(false); aqm::publish_measurement(sample); pump(); assert(sends == 2);
    subscribe_client(true); pump(); assert(sends == 2);
    aqm::publish_measurement({}); pump(); assert(sends == 3 && notification.size == 0);
    fail_alloc = true; aqm::publish_measurement(sample); pump(); assert(sends == 3); fail_alloc = false;
    fail_send = true; aqm::publish_measurement(sample); pump(); assert(sends == 4); fail_send = false;
    aqm::publish_measurement(sample); aqm::publish_measurement(sample); pump(); assert(sends == 5); // coalesced
    aqm::publish_measurement(sample); disconnect_client(); connect_client(8); subscribe_client(true);
    pump(); assert(sends == 5); // old event cannot target new connection
    aqm::publish_measurement(sample); pump(); assert(sends == 6);
    read = {}; clock_us = 14999999; assert(access(0, 0, &context, nullptr) == 0 && read.size == 5);
    clock_us = 15000000; read = {}; assert(access(0, 0, &context, nullptr) == 0 && read.size == 0);
    // Simulate a future schema payload to exercise exact transport capacity independently.
    aqm::EncodedMeasurement future; future.size = 20; snapshot.replace(future, clock_us); ++generation;
    notify(nullptr); assert(sends == 7);
    future.size = 21; snapshot.replace(future, clock_us); ++generation; notify(nullptr); assert(sends == 7);
    ble_gap_event event = {}; event.type = BLE_GAP_EVENT_MTU; event.mtu.conn_handle = connection; event.mtu.value = 80;
    event.mtu.conn_handle = 999; gap_event(&event, nullptr); assert(mtu == 23);
    event.mtu.conn_handle = connection; gap_event(&event, nullptr); assert(mtu == 80);
    // Size-only testing: oversize rejection occurs before bytes are read.
    future.size = 78; snapshot.replace(future, clock_us); ++generation; notify(nullptr); assert(sends == 7);
    future.size = 21; snapshot.replace(future, clock_us); ++generation; notify(nullptr); assert(sends == 8);
    fail_adv = true; disconnect_client(); assert(advertising_retry.active);
    fail_adv = false; advertising_retry.event.fn(nullptr); assert(advertising);
    on_reset(1); assert(!synchronized && !subscribed && connection == BLE_HS_CONN_HANDLE_NONE);
    advertising = false; on_sync(); assert(advertising);
    std::puts("BLE transport and lifecycle tests passed");
}
