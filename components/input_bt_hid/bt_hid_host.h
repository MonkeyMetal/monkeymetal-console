/// @file bt_hid_host.h
/// @brief MonkeyMetal BLE HID Gamepad Host — M5
///
/// Scans for BLE HID gamepads (8BitDo / Xbox / Switch Pro),
/// connects, subscribes to input reports, parses button state.

#pragma once

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

// Bluetooth Device Address: 6 bytes. Mirrors esp_bt_defs.h's esp_bd_addr_t
// but avoids pulling BT private headers into every includer.
typedef uint8_t bt_hid_bda_t[6];

// Forward: scan results carry 6-byte BDA + name + RSSI
typedef struct {
    bt_hid_bda_t bda;
    char name[32];
    int8_t rssi;
    uint8_t addr_type;  // BLE_ADDR_TYPE_PUBLIC or BLE_ADDR_TYPE_RANDOM
} bt_hid_scan_result_t;

// Logical button IDs — shared between BT host and Lua input binding
typedef enum {
    BTN_A = 0, BTN_B, BTN_X, BTN_Y,
    BTN_UP, BTN_DOWN, BTN_LEFT, BTN_RIGHT,
    BTN_START, BTN_SELECT, BTN_SYSTEM,
    BTN_LB, BTN_RB,  // d[13] bit6/bit7
    BTN_MAX
} bt_hid_button_t;

// Per-frame input state with edge detection.
//   cur[]      = current button state (refreshed on every BLE report)
//   prev[]     = cur snapshot taken at end-of-frame (for pressed/released)
//   latched[]  = OR-accumulated press events between frames; consumed by snap.
//                Prevents press events from being lost when BLE report rate
//                (60Hz+) exceeds Lua frame rate (~25fps) — a tap that goes
//                down+up within a single frame would otherwise show cur=false.
typedef struct {
    bool cur[BTN_MAX];
    bool prev[BTN_MAX];
    bool latched[BTN_MAX];
    int16_t ls_x;   // left stick X  (-32768 .. 32767)
    int16_t ls_y;   // left stick Y
    int16_t rs_x;   // right stick X
    int16_t rs_y;   // right stick Y
} bt_hid_input_state_t;

#ifdef __cplusplus
extern "C" {
#endif

// ── Public API ─────────────────────────────────────────────────────────────

/// @brief Initialize Bluedroid BLE + GATT client. Must call once per boot.
esp_err_t bt_hid_init(void);

/// @brief Start BLE scanning for HID devices.
esp_err_t bt_hid_scan_start(int timeout_sec);

/// @brief Stop scanning.
esp_err_t bt_hid_scan_stop(void);

/// @brief Connect to a device by BDA + address type.
/// @param addr_type BLE_ADDR_TYPE_PUBLIC (0) or BLE_ADDR_TYPE_RANDOM (1)
esp_err_t bt_hid_connect_ex(const bt_hid_bda_t bda, uint8_t addr_type);

/// @brief Disconnect current device.
esp_err_t bt_hid_disconnect(void);

/// @brief Snapshot cur → prev for edge detection. Call once per frame.
void bt_hid_input_poll(void);

/// @brief Read current input state without modifying edge info.
const bt_hid_input_state_t *bt_hid_input_read(void);

/// @brief Get current left stick X value (-32768..32767).
int16_t bt_hid_get_ls_x(void);

/// @brief Get current left stick Y value (-32768..32767).
int16_t bt_hid_get_ls_y(void);

/// @brief Get current right stick X value (-32768..32767).
int16_t bt_hid_get_rs_x(void);

/// @brief Get current right stick Y value (-32768..32767).
int16_t bt_hid_get_rs_y(void);

/// @brief Get scan results. Returns count, fills array (max N).
int bt_hid_get_scan_results(bt_hid_scan_result_t *out, int max_out);

/// @brief Check if a gamepad is currently connected.
bool bt_hid_is_connected(void);

/// @brief Deinitialize BT stack.
void bt_hid_deinit(void);

/// @brief Try to reconnect to the last bonded device from NVS.
bool bt_hid_reconnect_bonded(void);

/// @brief Check if auto-reconnect is needed and try it.
/// Call periodically from main loop (every 2-3 seconds).
void bt_hid_poll_reconnect(void);

#ifdef __cplusplus
}
#endif
