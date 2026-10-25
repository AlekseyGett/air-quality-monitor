#pragma once
#include <stdint.h>
#include <stddef.h>
struct ble_uuid_t { uint8_t type; };
struct ble_uuid16_t { ble_uuid_t u; uint16_t value; };
struct ble_uuid128_t { ble_uuid_t u; uint8_t value[16]; };
#define BLE_UUID16_INIT(v) {{16}, v}
#define BLE_UUID128_INIT(...) {{128}, {__VA_ARGS__}}
struct os_mbuf { uint8_t bytes[128]; size_t size; };
struct ble_gatt_access_ctxt { uint8_t op; os_mbuf *om; uint16_t offset; };
typedef int (*access_fn)(uint16_t, uint16_t, ble_gatt_access_ctxt *, void *);
struct ble_gatt_chr_def { const ble_uuid_t *uuid; access_fn access_cb; void *arg; uint16_t flags; uint16_t *val_handle; };
struct ble_gatt_svc_def { uint8_t type; const ble_uuid_t *uuid; const ble_gatt_chr_def *characteristics; };
struct ble_npl_event { void (*fn)(ble_npl_event *); bool queued; };
struct ble_npl_eventq { int unused; };
struct ble_npl_callout { ble_npl_event event; bool active; };
struct ble_gap_event
{
    int type;
    struct { int status; uint16_t conn_handle; } connect;
    struct { uint16_t conn_handle, attr_handle; bool cur_notify; } subscribe;
    struct { uint16_t conn_handle, value; } mtu;
};
struct ble_hs_adv_fields
{
    uint8_t flags; const ble_uuid128_t *uuids128;
    uint8_t num_uuids128, uuids128_is_complete;
    const uint8_t *name; uint8_t name_len, name_is_complete;
};
struct ble_gap_adv_params { int conn_mode, disc_mode; };
struct fake_hs_cfg { void (*reset_cb)(int); void (*sync_cb)(); int sm_bonding, sm_mitm; };
extern fake_hs_cfg ble_hs_cfg;
#define BLE_HS_CONN_HANDLE_NONE 65535
#define BLE_GATT_ACCESS_OP_READ_CHR 0
#define BLE_ATT_ERR_READ_NOT_PERMITTED 2
#define BLE_ATT_ERR_INSUFFICIENT_RES 17
#define BLE_GATT_CHR_F_READ 2
#define BLE_GATT_CHR_F_NOTIFY 16
#define BLE_GATT_SVC_TYPE_PRIMARY 1
#define BLE_HS_ENOMEM 6
#define BLE_HS_ADV_F_DISC_GEN 2
#define BLE_HS_ADV_F_BREDR_UNSUP 4
#define BLE_HS_FOREVER -1
#define BLE_GAP_CONN_MODE_UND 2
#define BLE_GAP_DISC_MODE_GEN 2
#define BLE_GAP_EVENT_CONNECT 1
#define BLE_GAP_EVENT_DISCONNECT 2
#define BLE_GAP_EVENT_SUBSCRIBE 3
#define BLE_GAP_EVENT_MTU 4
#define BLE_GAP_EVENT_ADV_COMPLETE 5
int nimble_port_init();
int nimble_port_deinit();
void nimble_port_run();
ble_npl_eventq *nimble_port_get_dflt_eventq();
int os_mbuf_append(os_mbuf *, const void *, size_t);
os_mbuf *ble_hs_mbuf_from_flat(const void *, size_t);
int ble_gatts_notify_custom(uint16_t, uint16_t, os_mbuf *);
int ble_gap_adv_active();
int ble_gap_adv_set_fields(const ble_hs_adv_fields *);
int ble_gap_adv_rsp_set_fields(const ble_hs_adv_fields *);
int ble_gap_adv_start(uint8_t, const void *, int, const ble_gap_adv_params *, int (*)(ble_gap_event *, void *), void *);
int ble_hs_util_ensure_addr(int);
int ble_hs_id_infer_auto(int, uint8_t *);
void ble_svc_gap_init();
void ble_svc_gatt_init();
int ble_svc_gap_device_name_set(const char *);
int ble_att_set_preferred_mtu(uint16_t);
int ble_gatts_count_cfg(const ble_gatt_svc_def *);
int ble_gatts_add_svcs(const ble_gatt_svc_def *);
void ble_npl_event_init(ble_npl_event *, void (*)(ble_npl_event *), void *);
void ble_npl_event_deinit(ble_npl_event *);
int ble_npl_callout_init(ble_npl_callout *, ble_npl_eventq *, void (*)(ble_npl_event *), void *);
void ble_npl_callout_deinit(ble_npl_callout *);
int ble_npl_callout_reset(ble_npl_callout *, uint32_t);
void ble_npl_callout_stop(ble_npl_callout *);
uint32_t ble_npl_time_ms_to_ticks32(uint32_t);
void ble_npl_eventq_put(ble_npl_eventq *, ble_npl_event *);
