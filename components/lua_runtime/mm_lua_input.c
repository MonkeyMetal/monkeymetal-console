/// @file mm_lua_input.c
/// @brief Lua binding for input (input.* table)
///
/// M5: BLE HID gamepad + BOOT key (GPIO 0) fallback.
///
/// Lua API:
///   input.down(btn)      true if button held this frame
///   input.pressed(btn)   true if button just went down (edge)
///   input.released(btn)  true if button just went up (edge)
///
/// Button names: "a" "b" "up" "down" "left" "right"
///               "x" "y" "start" "select" "system"

#include "lua.h"
#include "lauxlib.h"
#include "driver/gpio.h"
#include "bt_hid_host.h"
#include "esp_log.h"
#include <string.h>
#include <stdbool.h>

static const char *TAG = "lua_input";

#define BOOT_GPIO   GPIO_NUM_0

static bool s_boot_prev = false;
static bool s_boot_cur  = false;
static bool s_boot_armed = true;  // must see a release before BOOT counts as held
static bool s_inited    = false;

// ── Button name → BT HID enum ───────────────────────────────────────────────
static int btn_index(const char *name)
{
    if (!strcmp(name, "a"))      return BTN_A;
    if (!strcmp(name, "b"))      return BTN_B;
    if (!strcmp(name, "x"))      return BTN_X;
    if (!strcmp(name, "y"))      return BTN_Y;
    if (!strcmp(name, "up"))     return BTN_UP;
    if (!strcmp(name, "down"))   return BTN_DOWN;
    if (!strcmp(name, "left"))   return BTN_LEFT;
    if (!strcmp(name, "right"))  return BTN_RIGHT;
    if (!strcmp(name, "start"))  return BTN_START;
    if (!strcmp(name, "select")) return BTN_SELECT;
    if (!strcmp(name, "system")) return BTN_SYSTEM;
    if (!strcmp(name, "lb"))     return BTN_LB;
    if (!strcmp(name, "rb"))     return BTN_RB;
    return -1;
}

// ── Init ────────────────────────────────────────────────────────────────────
static void input_init(void)
{
    if (s_inited) return;
    gpio_config_t cfg = {
        .pin_bit_mask = (1ULL << BOOT_GPIO),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&cfg);
    s_inited = true;
    ESP_LOGI(TAG, "Input: BOOT key (GPIO%d) + BLE HID gamepad", BOOT_GPIO);
}

// ── Poll ────────────────────────────────────────────────────────────────────
// Called at the START of each frame:
//   1. snap last-frame BOOT state into prev,
//   2. read fresh BOOT state into cur,
//   3. snap last-frame BT cur into prev + merge any latched taps into cur.
// update() then sees consistent (prev, cur) for pressed()/released() edges.
void mm_input_poll(void)
{
    if (!s_inited) input_init();

    s_boot_prev = s_boot_cur;
    s_boot_cur  = (gpio_get_level(BOOT_GPIO) == 0);

    // BOOT auto-arm: once released, future presses count again.
    if (!s_boot_cur) s_boot_armed = true;

    bt_hid_input_poll(); // prev = cur; merge latched into cur

    // Auto-reconnect check (rate-limited internally)
    bt_hid_poll_reconnect();
}

// Kept as a no-op for backward compat with code that still calls it at frame end.
void mm_input_snap(void) { }

// Called by runtime on cart switch: a still-held BOOT/SELECT carried from the
// previous cart must NOT immediately re-trigger the global hotkey on the new
// cart. Arm flags get reset; held buttons must be released first.
void mm_input_reset_hotkeys(void)
{
    s_boot_armed = false;
    // Mark BT SELECT/START "prev" as held so the next pressed() edge requires
    // a release-then-press cycle. We flip prev to match cur so no rising edge
    // can fire until cur actually drops.
    const bt_hid_input_state_t *bt = bt_hid_input_read();
    (void)bt;
    // Use the public latched-clear path: poll once to merge anything pending,
    // then mark prev=cur via another poll-snapshot. The latched array gets
    // cleared inside bt_hid_input_poll, which is enough.
    bt_hid_input_poll();
}

bool mm_input_boot_held(void)
{
    // Only count as "held" once BOOT has been seen released since last cart.
    return s_boot_cur && s_boot_armed;
}

bool mm_input_select_pressed(void)
{
    const bt_hid_input_state_t *bt = bt_hid_input_read();
    return bt->cur[BTN_SELECT] && !bt->prev[BTN_SELECT];
}

// ── Query per-button state ──────────────────────────────────────────────────
// BOOT key is reserved for the global "back to launcher" long-hold gesture.
// Mapping it to A makes game controls fight with the system exit gesture,
// so BOOT only feeds "system" — carts that want a software A fallback can
// listen on "system" themselves.
static bool btn_down(int idx)
{
    if (idx < 0 || idx >= BTN_MAX) return false;
    const bt_hid_input_state_t *bt = bt_hid_input_read();

    if (idx == BTN_SYSTEM) {
        return bt->cur[idx] || s_boot_cur;
    }
    return bt->cur[idx];
}

static bool btn_pressed(int idx)
{
    if (idx < 0 || idx >= BTN_MAX) return false;
    const bt_hid_input_state_t *bt = bt_hid_input_read();

    if (idx == BTN_SYSTEM) {
        return (bt->cur[idx] && !bt->prev[idx]) ||
               (s_boot_cur && !s_boot_prev);
    }
    return bt->cur[idx] && !bt->prev[idx];
}

static bool btn_released(int idx)
{
    if (idx < 0 || idx >= BTN_MAX) return false;
    const bt_hid_input_state_t *bt = bt_hid_input_read();

    if (idx == BTN_SYSTEM) {
        return (!bt->cur[idx] && bt->prev[idx]) ||
               (!s_boot_cur && s_boot_prev);
    }
    return !bt->cur[idx] && bt->prev[idx];
}

// ── Lua functions ───────────────────────────────────────────────────────────
static int l_input_down(lua_State *L)
{
    int idx = btn_index(luaL_checkstring(L, 1));
    lua_pushboolean(L, btn_down(idx));
    return 1;
}

static int l_input_pressed(lua_State *L)
{
    int idx = btn_index(luaL_checkstring(L, 1));
    lua_pushboolean(L, btn_pressed(idx));
    return 1;
}

static int l_input_released(lua_State *L)
{
    int idx = btn_index(luaL_checkstring(L, 1));
    lua_pushboolean(L, btn_released(idx));
    return 1;
}

static int l_input_ls_x(lua_State *L)
{
    (void)L;
    lua_pushinteger(L, bt_hid_get_ls_x());
    return 1;
}

static int l_input_ls_y(lua_State *L)
{
    (void)L;
    lua_pushinteger(L, bt_hid_get_ls_y());
    return 1;
}

static int l_input_rs_x(lua_State *L)
{
    (void)L;
    lua_pushinteger(L, bt_hid_get_rs_x());
    return 1;
}

static int l_input_rs_y(lua_State *L)
{
    (void)L;
    lua_pushinteger(L, bt_hid_get_rs_y());
    return 1;
}

static const luaL_Reg input_funcs[] = {
    {"down",     l_input_down},
    {"pressed",  l_input_pressed},
    {"released", l_input_released},
    {"ls_x",     l_input_ls_x},
    {"ls_y",     l_input_ls_y},
    {"rs_x",     l_input_rs_x},
    {"rs_y",     l_input_rs_y},
    {NULL, NULL}
};

int mm_lua_open_input(lua_State *L)
{
    input_init();
    luaL_newlib(L, input_funcs);
    return 1;
}
