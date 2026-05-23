/// @file mm_lua_input.c
/// @brief Lua binding for input (input.* table)
///
/// M2/M3: BOOT key (GPIO 0) only, mapped to button "a" and "system".
/// M5: BLE HID host replaces/extends this.
///
/// Lua API:
///   input.down(btn)      true if button held this frame
///   input.pressed(btn)   true if button just went down (edge)
///   input.released(btn)  true if button just went up (edge)
///
/// Button names: "a" "b" "up" "down" "left" "right"
///               "x" "y" "start" "select" "system"
/// (M2: only "a" and "system" are wired to the BOOT key)

#include "lua.h"
#include "lauxlib.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include <string.h>
#include <stdbool.h>

static const char *TAG = "lua_input";

#define BOOT_GPIO   GPIO_NUM_0

// State for edge detection
static bool s_prev_boot = false;
static bool s_cur_boot  = false;
static bool s_initialized = false;

static void input_init(void)
{
    if (s_initialized) return;
    gpio_config_t cfg = {
        .pin_bit_mask = (1ULL << BOOT_GPIO),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&cfg);
    s_initialized = true;
    ESP_LOGI(TAG, "Input: BOOT key (GPIO%d) initialized", BOOT_GPIO);
}

// Called once per frame by the runtime (before update())
// Updates prev/cur state for edge detection.
void mm_input_poll(void)
{
    if (!s_initialized) input_init();
    s_prev_boot = s_cur_boot;
    // GPIO low = pressed (active-low with pullup)
    s_cur_boot = (gpio_get_level(BOOT_GPIO) == 0);
}

// Map button name string → bool (is this the BOOT key?)
static bool btn_is_boot(const char *name)
{
    return strcmp(name, "a") == 0 ||
           strcmp(name, "system") == 0;
}

static int l_input_down(lua_State *L)
{
    const char *btn = luaL_checkstring(L, 1);
    if (btn_is_boot(btn)) {
        lua_pushboolean(L, s_cur_boot);
    } else {
        lua_pushboolean(L, false);
    }
    return 1;
}

static int l_input_pressed(lua_State *L)
{
    const char *btn = luaL_checkstring(L, 1);
    if (btn_is_boot(btn)) {
        lua_pushboolean(L, s_cur_boot && !s_prev_boot);
    } else {
        lua_pushboolean(L, false);
    }
    return 1;
}

static int l_input_released(lua_State *L)
{
    const char *btn = luaL_checkstring(L, 1);
    if (btn_is_boot(btn)) {
        lua_pushboolean(L, !s_cur_boot && s_prev_boot);
    } else {
        lua_pushboolean(L, false);
    }
    return 1;
}

static const luaL_Reg input_funcs[] = {
    {"down",     l_input_down},
    {"pressed",  l_input_pressed},
    {"released", l_input_released},
    {NULL, NULL}
};

int mm_lua_open_input(lua_State *L)
{
    input_init();
    luaL_newlib(L, input_funcs);
    return 1;
}
