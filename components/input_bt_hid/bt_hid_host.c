/// @file bt_hid_host.c
/// @brief BLE HID Gamepad Host — GATT client implementation

#include "bt_hid_host.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <stdio.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <esp_log.h>
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_gattc_api.h"
#include "esp_gatt_common_api.h"
#include "esp_bt_device.h"
#include "esp_timer.h"

static const char *TAG = "bt_hid";

// ── Constants ───────────────────────────────────────────────────────────────
#define HID_SVC_UUID        0x1812
#define HID_REPORT_UUID     0x2A4D
#define CCCD_UUID           0x2902
#define MAX_SCAN_RESULTS    16
#define SCAN_TIMEOUT_DEFAULT 30   // seconds
#define NVS_NAMESPACE       "bt_hid"
#define NVS_KEY_BONDED      "bonded_bda"

// ── GATT profile state ──────────────────────────────────────────────────────
#define PROFILE_APP_ID  0
#define PROFILE_NUM     1

enum {
    GATTC_IDX_SVC_HID = 0,
    GATTC_IDX_CHAR_HID_REPORT,
    GATTC_IDX_HID_REPORT_VAL,
    GATTC_IDX_HID_REPORT_CFG,
    GATTC_IDX_MAX
};

// ── Internal state ──────────────────────────────────────────────────────────
static bt_hid_input_state_t s_input;
static bt_hid_bda_t         s_peer_bda;  // connected device BDA
static uint8_t              s_conn_id    = 0;
#define MAX_REPORT_HANDLES 8
static uint16_t             s_hid_report_handles[MAX_REPORT_HANDLES] = {0};
static int                  s_hid_report_count  = 0;
static int                  s_hid_notify_idx    = 0;  // for sequential registration
static uint16_t             s_hid_ctrl_handle   = 0;
static uint16_t             s_hid_proto_handle  = 0;
static bool                 s_connected  = false;
static bool                 s_scanning   = false;
static bool                 s_auto_reconnect = true;  // re-enable after disconnect
static bool                 s_terminal_sel = false;

// Scan results
static bt_hid_scan_result_t s_scan_results[MAX_SCAN_RESULTS];
static int                  s_scan_count = 0;

// Event group for sync operations
static EventGroupHandle_t   s_evt_group;


// ── Forward decls ───────────────────────────────────────────────────────────
static void gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t iface,
                                esp_ble_gattc_cb_param_t *param);
static void gap_event_handler(esp_gap_ble_cb_event_t event,
                              esp_ble_gap_cb_param_t *param);

// ── Critical section protection for input state ─────────────────────────────
static portMUX_TYPE s_input_lock = portMUX_INITIALIZER_UNLOCKED;

// ── GATT profile ────────────────────────────────────────────────────────────

// ── Initialize ──────────────────────────────────────────────────────────────
esp_err_t bt_hid_init(void)
{
    s_evt_group = xEventGroupCreate();
    if (!s_evt_group) return ESP_ERR_NO_MEM;

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_BLE));
    ESP_ERROR_CHECK(esp_bluedroid_init());
    ESP_ERROR_CHECK(esp_bluedroid_enable());

    // ── BLE Security: set up SMP pairing for gamepads ──────────────────
    // Xbox/Switch/8BitDo all require bonding with "No Input No Output" IO cap.
    uint8_t iocap    = ESP_IO_CAP_NONE;          // No keyboard/display on console
    uint8_t auth_req = ESP_LE_AUTH_REQ_SC_BOND;  // Secure Connections + Bonding
    uint8_t key_size = 16;
    uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    uint8_t rsp_key  = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE,       &iocap,    1);
    esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE,  &auth_req, 1);
    esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE,     &key_size, 1);
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY,     &init_key, 1);
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY,      &rsp_key,  1);

    // Register callbacks
    ESP_ERROR_CHECK(esp_ble_gap_register_callback(gap_event_handler));
    ESP_ERROR_CHECK(esp_ble_gattc_register_callback(gattc_event_handler));

    // Create GATT client application profile
    ESP_ERROR_CHECK(esp_ble_gattc_app_register(PROFILE_APP_ID));

    // Set local MTU
    ESP_ERROR_CHECK(esp_ble_gatt_set_local_mtu(200));

    // Wait for GATT registration to complete
    EventBits_t bits = xEventGroupWaitBits(s_evt_group, BIT0, pdTRUE, pdFALSE,
                                           pdMS_TO_TICKS(3000));
    if (!(bits & BIT0)) {
        ESP_LOGE(TAG, "GATT registration timed out");
        return ESP_ERR_TIMEOUT;
    }

    ESP_LOGI(TAG, "BLE HID Host initialized");
    return ESP_OK;
}

// ── Scan ─────────────────────────────────────────────────────────────────────
static void gap_event_handler(esp_gap_ble_cb_event_t event,
                              esp_ble_gap_cb_param_t *param)
{
    switch (event) {
    case ESP_GAP_BLE_SCAN_RESULT_EVT: {
        esp_ble_gap_cb_param_t *r = (esp_ble_gap_cb_param_t *)param;
        uint8_t *adv_name = NULL;
        uint8_t adv_name_len = 0;
        adv_name = esp_ble_resolve_adv_data(r->scan_rst.ble_adv,
                                            ESP_BLE_AD_TYPE_NAME_CMPL, &adv_name_len);
        // Also check for short name
        if (!adv_name) {
            adv_name = esp_ble_resolve_adv_data(r->scan_rst.ble_adv,
                                                ESP_BLE_AD_TYPE_NAME_SHORT, &adv_name_len);
        }

        if (adv_name && s_scan_count < MAX_SCAN_RESULTS) {
            // Check for duplicates
            for (int i = 0; i < s_scan_count; i++) {
                if (memcmp(s_scan_results[i].bda, r->scan_rst.bda, 6) == 0) {
                    s_scan_results[i].rssi = r->scan_rst.rssi;
                    return; // update RSSI, don't add
                }
            }
            // New device
            memcpy(s_scan_results[s_scan_count].bda, r->scan_rst.bda, 6);
            int nlen = adv_name_len < 31 ? adv_name_len : 31;
            memcpy(s_scan_results[s_scan_count].name, adv_name, nlen);
            s_scan_results[s_scan_count].name[nlen] = '\0';
            s_scan_results[s_scan_count].rssi = r->scan_rst.rssi;
            s_scan_results[s_scan_count].addr_type = r->scan_rst.ble_addr_type;
            s_scan_count++;
            ESP_LOGI(TAG, "Found: %s (RSSI %d, type=%d)",
                     s_scan_results[s_scan_count-1].name,
                     s_scan_results[s_scan_count-1].rssi,
                     s_scan_results[s_scan_count-1].addr_type);
        }
        break;
    }
    case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:
        s_scanning = false;
        ESP_LOGI(TAG, "Scan stopped. Found %d devices", s_scan_count);
        break;

    case ESP_GAP_BLE_SEC_REQ_EVT:
        // Gamepad requests security pairing — accept and encrypt
        ESP_LOGI(TAG, "Security request from peer, encrypting...");
        esp_ble_set_encryption(param->ble_security.ble_req.bd_addr,
                                ESP_BLE_SEC_ENCRYPT);
        break;

    case ESP_GAP_BLE_AUTH_CMPL_EVT: {
        esp_ble_auth_cmpl_t *auth = &param->ble_security.auth_cmpl;
        if (auth->success) {
            ESP_LOGI(TAG, "Pairing OK: %02x:%02x:%02x:%02x:%02x:%02x",
                     auth->bd_addr[0], auth->bd_addr[1], auth->bd_addr[2],
                     auth->bd_addr[3], auth->bd_addr[4], auth->bd_addr[5]);
        } else {
            ESP_LOGW(TAG, "Pairing failed: reason=0x%x", auth->fail_reason);
        }
        break;
    }

    default:
        break;
    }
}

esp_err_t bt_hid_scan_start(int timeout_sec)
{
    if (s_scanning) return ESP_OK;
    s_scan_count = 0;
    memset(s_scan_results, 0, sizeof(s_scan_results));

    esp_ble_scan_params_t scan_params = {
        .scan_type              = BLE_SCAN_TYPE_ACTIVE,
        .own_addr_type          = BLE_ADDR_TYPE_PUBLIC,
        .scan_filter_policy     = BLE_SCAN_FILTER_ALLOW_ALL,
        .scan_interval          = 0x50,    // 50 ms
        .scan_window            = 0x30,    // 30 ms
        .scan_duplicate         = BLE_SCAN_DUPLICATE_DISABLE,
    };
    ESP_ERROR_CHECK(esp_ble_gap_set_scan_params(&scan_params));
    ESP_ERROR_CHECK(esp_ble_gap_start_scanning(timeout_sec > 0 ? timeout_sec : SCAN_TIMEOUT_DEFAULT));
    s_scanning = true;
    ESP_LOGI(TAG, "Scan started (timeout %ds)", timeout_sec > 0 ? timeout_sec : SCAN_TIMEOUT_DEFAULT);
    return ESP_OK;
}

esp_err_t bt_hid_scan_stop(void)
{
    if (!s_scanning) return ESP_OK;
    ESP_ERROR_CHECK(esp_ble_gap_stop_scanning());
    return ESP_OK;
}

int bt_hid_get_scan_results(bt_hid_scan_result_t *out, int max_out)
{
    int n = (s_scan_count < max_out) ? s_scan_count : max_out;
    memcpy(out, s_scan_results, n * sizeof(bt_hid_scan_result_t));
    return n;
}

// ── Connect / Disconnect ────────────────────────────────────────────────────
static esp_gatt_if_t s_gattc_if = ESP_GATT_IF_NONE;

esp_err_t bt_hid_connect_ex(const bt_hid_bda_t bda, uint8_t addr_type)
{
    if (s_connected) {
        bt_hid_disconnect();
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    memcpy(s_peer_bda, bda, sizeof(bt_hid_bda_t));
    ESP_LOGI(TAG, "Connecting to %02x:%02x:%02x:%02x:%02x:%02x (type=%d)",
             bda[0], bda[1], bda[2], bda[3], bda[4], bda[5], addr_type);

    esp_ble_gap_set_prefer_conn_params((uint8_t *)bda, 8, 16, 0, 3200);
    // min_int=10ms, max_int=20ms, latency=0, timeout=32s (max BLE)
    esp_err_t ret = esp_ble_gattc_open(s_gattc_if, (uint8_t *)bda,
                                       (esp_ble_addr_type_t)addr_type, true);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initiate connection: %s", esp_err_to_name(ret));
        return ret;
    }

    EventBits_t bits = xEventGroupWaitBits(s_evt_group, BIT1, pdTRUE, pdFALSE,
                                           pdMS_TO_TICKS(15000));
    if (!(bits & BIT1)) {
        ESP_LOGE(TAG, "Connection timeout");
        return ESP_ERR_TIMEOUT;
    }

    nvs_handle_t nvs;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs) == ESP_OK) {
        nvs_set_blob(nvs, NVS_KEY_BONDED, bda, sizeof(bt_hid_bda_t));
        nvs_commit(nvs);
        nvs_close(nvs);
    }

    ESP_LOGI(TAG, "Connected + bonded");
    return ESP_OK;
}

esp_err_t bt_hid_disconnect(void)
{
    if (!s_connected) return ESP_OK;
    esp_ble_gattc_close(s_gattc_if, s_conn_id);
    s_connected = false;
    memset(s_hid_report_handles, 0, sizeof(s_hid_report_handles));
    s_hid_report_count = 0;
    s_hid_ctrl_handle = 0;
    ESP_LOGI(TAG, "Disconnected");
    return ESP_OK;
}

bool bt_hid_is_connected(void) { return s_connected; }

// ── Auto-reconnect poll ───────────────────────────────────────────────────
void bt_hid_poll_reconnect(void)
{
    static int64_t last_try = 0;
    if (!s_connected && s_auto_reconnect && !s_scanning) {
        int64_t now = esp_timer_get_time();
        if (now - last_try < 5000000) return; // 5s between attempts
        last_try = now;
        s_auto_reconnect = false;
        ESP_LOGI(TAG, "Trying auto-reconnect...");
        if (!bt_hid_reconnect_bonded()) {
            bt_hid_scan_start(15);
        }
    }
}

// ── HID Report Parsing ──────────────────────────────────────────────────────

/// Standard BLE HID Gamepad input report (most common format):
///   Byte 0: Report ID (typically 0x01 or 0x03)
///   Byte 1: Buttons low  (bit 0=A, 1=B, 2=X, 3=Y, 4=LB, 5=RB, etc.)
///   Byte 2: Buttons high
///   Byte 3: D-Pad (0=center, 1=up, 2=up-right, ..., 8 cycles CW through 8 dirs)
/// Some controllers swap or omit fields. We try common layouts.
static void parse_gamepad_report(const uint8_t *data, int len)
{
    // Xbox BLE 16-byte report — final mapping 2026-05-24:
    //   d[1-2]: Left stick X  (int16 LE)
    //   d[3-4]: Right stick Y
    //   d[5-6]: Left stick Y  (int16 LE)
    //   d[7-8]: Right stick X
    //   d[12]:  D-Pad hat switch: 0=center, 1=U, 2=UR, 3=R, 4=RD,
    //                               5=D, 6=DL, 7=L, 8=UL
    //   d[13]:  Button bitmap: bit0=A, bit1=B, bit3=X, bit4=Y,
    //                           bit6=LB, bit7=RB
    //   d[14]:  Stick clicks: bit5(0x20)=LS, bit6(0x40)=RS
    //           (View/Menu/Guide not available on BLE for this controller)

    if (len < 14) return;

    uint8_t dpad = data[12];
    uint8_t btn  = data[13];
    uint8_t misc = (len > 14) ? data[14] : 0;
    int16_t ls_x = (int16_t)(data[1] | ((uint16_t)data[2] << 8));
    int16_t ls_y = (int16_t)(data[5] | ((uint16_t)data[6] << 8));
    int16_t rs_x = (int16_t)(data[7] | ((uint16_t)data[8] << 8));
    int16_t rs_y = (int16_t)(data[3] | ((uint16_t)data[4] << 8));

    portENTER_CRITICAL(&s_input_lock);

    // Build new state into a local buffer first so we can compare to old cur.
    bool new_cur[BTN_MAX] = {0};

    s_input.ls_x = ls_x;
    s_input.ls_y = ls_y;
    s_input.rs_x = rs_x;
    s_input.rs_y = rs_y;

    // ── D-Pad (d[12]: hat switch 0-8) ──────────────────────────────────────
    if (dpad == 1 || dpad == 2 || dpad == 8) new_cur[BTN_UP]    = true;
    if (dpad == 3 || dpad == 2 || dpad == 4) new_cur[BTN_RIGHT] = true;
    if (dpad == 5 || dpad == 4 || dpad == 6) new_cur[BTN_DOWN]  = true;
    if (dpad == 7 || dpad == 6 || dpad == 8) new_cur[BTN_LEFT]  = true;

    // ── Face buttons (d[13]: bitmap) ───────────────────────────────────────
    if (btn & 0x01) new_cur[BTN_A]  = true;
    if (btn & 0x02) new_cur[BTN_B]  = true;
    if (btn & 0x08) new_cur[BTN_X]  = true;
    if (btn & 0x10) new_cur[BTN_Y]  = true;
    if (btn & 0x40) new_cur[BTN_LB] = true;
    if (btn & 0x80) new_cur[BTN_RB] = true;

    // ── Stick clicks (d[14]: LS/RS → SELECT/START) ─────────────────────────
    if (misc & 0x20) new_cur[BTN_SELECT] = true;  // LS click
    if (misc & 0x40) new_cur[BTN_START]  = true;  // RS click

    // Detect rising edges between two BLE reports and latch them so that a
    // tap whose down+up cycle fits between two Lua frame snaps still fires
    // pressed() exactly once. Consumed/cleared in bt_hid_input_poll().
    for (int i = 0; i < BTN_MAX; i++) {
        if (new_cur[i] && !s_input.cur[i]) s_input.latched[i] = true;
        s_input.cur[i] = new_cur[i];
    }

    portEXIT_CRITICAL(&s_input_lock);
}

// ── Input Poll / Read ───────────────────────────────────────────────────────

/// Called once per frame from main thread: snapshot cur → prev for edge detect.
/// Also folds the inter-frame latch into cur so brief taps fired between two
/// frame snaps still register as a press exactly once.
void bt_hid_input_poll(void)
{
    portENTER_CRITICAL(&s_input_lock);
    // prev = cur (for last frame's edge detection)
    memcpy(s_input.prev, s_input.cur, sizeof(s_input.cur));
    // Merge any latched press events (from between BLE reports) into cur so
    // pressed()/down() observe them this frame. Clear after merging.
    // Also reset prev to false for latched buttons so that pressed() detects
    // the rising edge: cur was already updated by the BLE callback before
    // this poll, so prev=cur would lose the edge without this fix.
    for (int i = 0; i < BTN_MAX; i++) {
        if (s_input.latched[i]) {
            s_input.cur[i] = true;
            s_input.prev[i] = false;
        }
        s_input.latched[i] = false;
    }
    portEXIT_CRITICAL(&s_input_lock);
}

/// Lightweight read of current state (no side effects).
const bt_hid_input_state_t *bt_hid_input_read(void)
{
    return &s_input;
}

int16_t bt_hid_get_ls_x(void) { return s_input.ls_x; }
int16_t bt_hid_get_ls_y(void) { return s_input.ls_y; }
int16_t bt_hid_get_rs_x(void) { return s_input.rs_x; }
int16_t bt_hid_get_rs_y(void) { return s_input.rs_y; }

// ── GATT Event Handler ──────────────────────────────────────────────────────
static void gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t iface,
                                esp_ble_gattc_cb_param_t *param)
{
    switch (event) {
    case ESP_GATTC_REG_EVT:
        if (param->reg.app_id == PROFILE_APP_ID) {
            s_gattc_if = iface;
            xEventGroupSetBits(s_evt_group, BIT0); // Registration done
            ESP_LOGI(TAG, "GATT client registered");
        }
        break;

    case ESP_GATTC_CONNECT_EVT:
        s_connected = true;
        s_conn_id = param->connect.conn_id;
        memcpy(s_peer_bda, param->connect.remote_bda, sizeof(bt_hid_bda_t));
        ESP_LOGI(TAG, "BLE connected, starting service discovery...");

        // Initiate encryption (SMP pairing + bonding) immediately
        // Xbox/Switch/etc. need this before they'll send HID reports
        esp_ble_set_encryption(param->connect.remote_bda, ESP_BLE_SEC_ENCRYPT);

        esp_ble_gattc_search_service(iface, param->connect.conn_id, NULL);
        break;

    case ESP_GATTC_DISCONNECT_EVT:
        s_connected = false;
        memset(s_hid_report_handles, 0, sizeof(s_hid_report_handles));
        s_hid_report_count = 0;
        s_hid_ctrl_handle = 0;
        s_hid_proto_handle = 0;
        s_auto_reconnect = true;
        ESP_LOGI(TAG, "BLE disconnected (will attempt reconnect)");
        break;

    case ESP_GATTC_SEARCH_RES_EVT:
        // Store service/char info for later lookup
        break;

    case ESP_GATTC_SEARCH_CMPL_EVT: {
        // ── Enumerate ALL services and ALL characteristics ──────────────
        uint16_t svc_count = 0;
        esp_ble_gattc_get_attr_count(iface, s_conn_id, ESP_GATT_DB_PRIMARY_SERVICE,
                                      0x0001, 0xFFFF, 0, &svc_count);
        if (svc_count == 0) {
            ESP_LOGE(TAG, "No services found");
            xEventGroupSetBits(s_evt_group, BIT1);
            break;
        }

        esp_gattc_service_elem_t *svcs = malloc(svc_count * sizeof(esp_gattc_service_elem_t));
        esp_ble_gattc_get_service(iface, s_conn_id, NULL, svcs, &svc_count, 0);

        s_hid_report_count = 0;
        s_hid_ctrl_handle = 0;
        s_hid_proto_handle = 0;

        ESP_LOGI(TAG, "=== ALL SERVICES (%d) ===", svc_count);

        for (int si = 0; si < svc_count; si++) {
            // Print service UUID
            if (svcs[si].uuid.len == ESP_UUID_LEN_16) {
                ESP_LOGI(TAG, "SVC #%d uuid=0x%04X start=0x%04X end=0x%04X",
                         si, svcs[si].uuid.uuid.uuid16,
                         svcs[si].start_handle, svcs[si].end_handle);
            } else {
                ESP_LOGI(TAG, "SVC #%d uuid=%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X start=0x%04X end=0x%04X",
                         si,
                         svcs[si].uuid.uuid.uuid128[15],svcs[si].uuid.uuid.uuid128[14],
                         svcs[si].uuid.uuid.uuid128[13],svcs[si].uuid.uuid.uuid128[12],
                         svcs[si].uuid.uuid.uuid128[11],svcs[si].uuid.uuid.uuid128[10],
                         svcs[si].uuid.uuid.uuid128[9],svcs[si].uuid.uuid.uuid128[8],
                         svcs[si].uuid.uuid.uuid128[7],svcs[si].uuid.uuid.uuid128[6],
                         svcs[si].uuid.uuid.uuid128[5],svcs[si].uuid.uuid.uuid128[4],
                         svcs[si].uuid.uuid.uuid128[3],svcs[si].uuid.uuid.uuid128[2],
                         svcs[si].uuid.uuid.uuid128[1],svcs[si].uuid.uuid.uuid128[0],
                         svcs[si].start_handle, svcs[si].end_handle);
            }

            // Get characteristics within this service
            uint16_t ch_count = 0;
            esp_ble_gattc_get_attr_count(iface, s_conn_id, ESP_GATT_DB_CHARACTERISTIC,
                                          svcs[si].start_handle, svcs[si].end_handle,
                                          0, &ch_count);
            if (ch_count == 0) continue;

            esp_gattc_char_elem_t *chs = malloc(ch_count * sizeof(esp_gattc_char_elem_t));
            esp_ble_gattc_get_all_char(iface, s_conn_id,
                                        svcs[si].start_handle, svcs[si].end_handle,
                                        chs, &ch_count, 0);

            for (int ci = 0; ci < ch_count; ci++) {
                uint8_t p = chs[ci].properties;
                char props[64] = {0};
                if (p & 0x10) strcat(props, " NOTIFY");
                if (p & 0x20) strcat(props, " INDICATE");
                if (p & 0x02) strcat(props, " READ");
                if (p & 0x08) strcat(props, " WRITE");
                if (p & 0x04) strcat(props, " WRITE_NR");

                if (chs[ci].uuid.len == ESP_UUID_LEN_16) {
                    ESP_LOGI(TAG, "  CHR uuid=0x%04X handle=0x%04X%s",
                             chs[ci].uuid.uuid.uuid16, chs[ci].char_handle, props);
                } else {
                    ESP_LOGI(TAG, "  CHR uuid=128 handle=0x%04X%s",
                             chs[ci].char_handle, props);
                }

                // Subscribe to ALL NOTIFY characteristics
                if ((p & (ESP_GATT_CHAR_PROP_BIT_NOTIFY | ESP_GATT_CHAR_PROP_BIT_INDICATE))
                    && s_hid_report_count < MAX_REPORT_HANDLES) {
                    s_hid_report_handles[s_hid_report_count++] = chs[ci].char_handle;
                }
                // Remember HID Control Point
                if (chs[ci].uuid.len == ESP_UUID_LEN_16 &&
                    chs[ci].uuid.uuid.uuid16 == 0x2A4C) {
                    s_hid_ctrl_handle = chs[ci].char_handle;
                }
                if (chs[ci].uuid.len == ESP_UUID_LEN_16 &&
                    chs[ci].uuid.uuid.uuid16 == 0x2A4E) {
                    s_hid_proto_handle = chs[ci].char_handle;
                }
            }
            free(chs);
        }

        ESP_LOGI(TAG, "=== END SERVICES ===");
        free(svcs);

        if (s_hid_report_count > 0) {
            s_hid_notify_idx = 0;
            esp_ble_gattc_register_for_notify(iface, s_peer_bda,
                                              s_hid_report_handles[0]);
            ESP_LOGI(TAG, "Register notify 1/%d (h=0x%04x)",
                     s_hid_report_count, s_hid_report_handles[0]);
        } else {
            ESP_LOGW(TAG, "No NOTIFY/INDICATE characteristics found");
        }

        xEventGroupSetBits(s_evt_group, BIT1); // Connection complete
        break;
    }

    case ESP_GATTC_REG_FOR_NOTIFY_EVT: {
        uint16_t h = param->reg_for_notify.handle;
        uint16_t cccd_val = 0x0001; // default NOTIFY; INDICATE would be 0x0002
        esp_ble_gattc_write_char_descr(
            iface, s_conn_id, h,
            sizeof(cccd_val), (uint8_t *)&cccd_val,
            ESP_GATT_WRITE_TYPE_NO_RSP,
            ESP_GATT_AUTH_REQ_NONE);
        ESP_LOGI(TAG, "CCCD write h=0x%04x ok (%d/%d)", h,
                 s_hid_notify_idx + 1, s_hid_report_count);

        s_hid_notify_idx++;
        if (s_hid_notify_idx < s_hid_report_count) {
            esp_ble_gattc_register_for_notify(iface, s_peer_bda,
                                              s_hid_report_handles[s_hid_notify_idx]);
        } else {
            if (s_hid_ctrl_handle) {
                uint8_t wake = 0x01;
                esp_ble_gattc_write_char(
                    iface, s_conn_id, s_hid_ctrl_handle,
                    1, &wake,
                    ESP_GATT_WRITE_TYPE_NO_RSP,
                    ESP_GATT_AUTH_REQ_NONE);
                ESP_LOGI(TAG, "Ctrl Point wake 0x%02x", wake);
            }
        }
        break;
    }

    case ESP_GATTC_NOTIFY_EVT:
        parse_gamepad_report(param->notify.value, param->notify.value_len);
        break;

    case ESP_GATTC_OPEN_EVT:
        if (param->open.status != ESP_GATT_OK) {
            ESP_LOGE(TAG, "Connection failed: 0x%x", param->open.status);
            xEventGroupSetBits(s_evt_group, BIT1); // Unblock waiters (with error)
        }
        break;

    default:
        break;
    }
}

// ── Reconnect Bonded ────────────────────────────────────────────────────────
bool bt_hid_reconnect_bonded(void)
{
    int num = esp_ble_get_bond_device_num();
    if (num == 0) {
        ESP_LOGI(TAG, "No bonded devices");
        return false;
    }

    esp_ble_bond_dev_t *dev_list = malloc(num * sizeof(esp_ble_bond_dev_t));
    esp_ble_get_bond_device_list(&num, dev_list);

    bool ok = false;
    for (int i = 0; i < num; i++) {
        bt_hid_bda_t bda;
        memcpy(bda, dev_list[i].bd_addr, sizeof(bt_hid_bda_t));
        ESP_LOGI(TAG, "Bonded: %02x:%02x:%02x:%02x:%02x:%02x",
                 bda[0], bda[1], bda[2], bda[3], bda[4], bda[5]);

        esp_err_t ret = bt_hid_connect_ex(bda, BLE_ADDR_TYPE_RANDOM);
        if (ret != ESP_OK) {
            ret = bt_hid_connect_ex(bda, BLE_ADDR_TYPE_PUBLIC);
        }
        if (ret == ESP_OK) {
            ok = true;
            break;
        }
    }

    free(dev_list);
    return ok;
}

// ── Terminal select (dev helper) ────────────────────────────────────────────
void bt_hid_set_terminal_select(bool enable) { s_terminal_sel = enable; }

// ── Deinit ──────────────────────────────────────────────────────────────────
void bt_hid_deinit(void)
{
    if (s_connected) bt_hid_disconnect();
    if (s_scanning)  bt_hid_scan_stop();
    esp_bluedroid_disable();
    esp_bluedroid_deinit();
    esp_bt_controller_disable();
    esp_bt_controller_deinit();
    ESP_LOGI(TAG, "BT deinitialized");
}
