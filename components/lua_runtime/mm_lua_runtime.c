/// @file mm_lua_runtime.c
/// @brief MonkeyMetal Lua 5.4 runtime: VM init, PSRAM allocator, cart loader

#include "mm_lua_runtime.h"
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <stdio.h>

// Binding registration prototypes (defined in mm_lua_gfx.c / input / system / audio)
int mm_lua_open_gfx(lua_State *L);
int mm_lua_open_input(lua_State *L);
int mm_lua_open_system(lua_State *L);
int mm_lua_open_audio(lua_State *L);

// Input poll / snap / hotkey helpers
void mm_input_poll(void);
void mm_input_snap(void);
void mm_input_reset_hotkeys(void);
bool mm_input_boot_held(void);
bool mm_input_select_pressed(void);

static const char *TAG = "lua_rt";

// Global Lua state (one VM per console, replaced on each cart load)
static lua_State *s_L = NULL;
static char s_next_cart[128] = {0};
static bool s_is_launcher = false;
static int  s_boot_hold  = 0;

// ── PSRAM allocator ──────────────────────────────────────────────────────────
// Lua's default allocator uses libc malloc which goes to internal RAM.
// Route everything through PSRAM.  Falls back to internal RAM if PSRAM full.
static void *psram_alloc(void *ud, void *ptr, size_t osize, size_t nsize)
{
    (void)ud; (void)osize;
    if (nsize == 0) {
        free(ptr);
        return NULL;
    }
    if (ptr) {
        return heap_caps_realloc(ptr, nsize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    }
    void *p = heap_caps_malloc(nsize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!p) {
        // Fallback to internal RAM (small allocations)
        p = malloc(nsize);
    }
    return p;
}

// ── VM init / deinit ─────────────────────────────────────────────────────────
esp_err_t mm_lua_init(void)
{
    if (s_L) {
        mm_lua_deinit();
    }

    s_L = lua_newstate(psram_alloc, NULL);
    if (!s_L) {
        ESP_LOGE(TAG, "Failed to create Lua state (out of PSRAM?)");
        return ESP_ERR_NO_MEM;
    }

    // Open safe standard libs only (no io/os/package — sandboxed)
    luaL_requiref(s_L, "_G",       luaopen_base,   1); lua_pop(s_L, 1);
    luaL_requiref(s_L, "table",    luaopen_table,  1); lua_pop(s_L, 1);
    luaL_requiref(s_L, "string",   luaopen_string, 1); lua_pop(s_L, 1);
    luaL_requiref(s_L, "math",     luaopen_math,   1); lua_pop(s_L, 1);
    luaL_requiref(s_L, "coroutine",luaopen_coroutine, 1); lua_pop(s_L, 1);
    luaL_requiref(s_L, "utf8",     luaopen_utf8,   1); lua_pop(s_L, 1);

    // MonkeyMetal engine bindings
    luaL_requiref(s_L, "gfx",    mm_lua_open_gfx,    1); lua_pop(s_L, 1);
    luaL_requiref(s_L, "input",  mm_lua_open_input,  1); lua_pop(s_L, 1);
    luaL_requiref(s_L, "system", mm_lua_open_system, 1); lua_pop(s_L, 1);
    luaL_requiref(s_L, "audio",  mm_lua_open_audio,  1); lua_pop(s_L, 1);

    // Restrict dangerous base functions
    lua_pushnil(s_L); lua_setglobal(s_L, "dofile");
    lua_pushnil(s_L); lua_setglobal(s_L, "loadfile");
    lua_pushnil(s_L); lua_setglobal(s_L, "require");

    ESP_LOGI(TAG, "Lua 5.4 VM ready (PSRAM allocator, sandboxed)");
    return ESP_OK;
}

void mm_lua_deinit(void)
{
    if (s_L) {
        lua_close(s_L);
        s_L = NULL;
        ESP_LOGI(TAG, "Lua VM closed");
    }
}

const char *mm_lua_get_next_cart(void)
{
    return s_next_cart[0] ? s_next_cart : NULL;
}

void mm_lua_clear_next_cart(void)
{
    s_next_cart[0] = '\0';
}

void mm_lua_set_next_cart(const char *path)
{
    strncpy(s_next_cart, path, sizeof(s_next_cart) - 1);
    s_next_cart[sizeof(s_next_cart) - 1] = '\0';
}

// Called by C-side global hotkey or Lua-side system.run_cart
void mm_lua_request_cart(const char *path)
{
    mm_lua_set_next_cart(path);
    if (s_L) {
        lua_pushboolean(s_L, true);
        lua_setglobal(s_L, "__mm_exit_requested");
    }
}

// ── Cart loader ──────────────────────────────────────────────────────────────
esp_err_t mm_lua_run_cart(const char *cart_dir)
{
    if (!s_L) {
        ESP_LOGE(TAG, "Call mm_lua_init() first");
        return ESP_ERR_INVALID_STATE;
    }

    // Detect launcher cart (for global hotkey suppression)
    s_is_launcher = (strstr(cart_dir, "/system/launcher") != NULL);
    s_boot_hold = 0;

    // Reset hotkey state: a still-held BOOT/SELECT from the previous cart
    // (e.g. launcher → A starts game while BOOT was still down) must NOT
    // immediately trigger "back to launcher" on the new cart.
    mm_input_reset_hotkeys();

    // Build path to cart.lua
    char cart_path[128];
    snprintf(cart_path, sizeof(cart_path), "%s/cart.lua", cart_dir);

    // Clear next-cart AFTER reading cart_dir (cart_dir may point into s_next_cart)
    s_next_cart[0] = '\0';

    ESP_LOGI(TAG, "Loading cart: %s", cart_path);

    // Load + execute the cart file (defines init/update/draw in global scope)
    if (luaL_loadfile(s_L, cart_path) != LUA_OK) {
        ESP_LOGE(TAG, "Load error: %s", lua_tostring(s_L, -1));
        lua_pop(s_L, 1);
        return ESP_FAIL;
    }
    if (lua_pcall(s_L, 0, 0, 0) != LUA_OK) {
        ESP_LOGE(TAG, "Exec error: %s", lua_tostring(s_L, -1));
        lua_pop(s_L, 1);
        return ESP_FAIL;
    }

    // Check required entry points
    lua_getglobal(s_L, "init");
    bool has_init = lua_isfunction(s_L, -1);
    lua_pop(s_L, 1);

    lua_getglobal(s_L, "update");
    bool has_update = lua_isfunction(s_L, -1);
    lua_pop(s_L, 1);

    lua_getglobal(s_L, "draw");
    bool has_draw = lua_isfunction(s_L, -1);
    lua_pop(s_L, 1);

    if (!has_update || !has_draw) {
        ESP_LOGE(TAG, "Cart missing update() or draw()");
        return ESP_FAIL;
    }

    // Call init() once
    if (has_init) {
        lua_getglobal(s_L, "init");
        if (lua_pcall(s_L, 0, 0, 0) != LUA_OK) {
            ESP_LOGE(TAG, "init() error: %s", lua_tostring(s_L, -1));
            lua_pop(s_L, 1);
            return ESP_FAIL;
        }
    }

    ESP_LOGI(TAG, "Cart running: %s", cart_dir);

    // Main game loop: update() + draw() + gfx.flip().  gfx_flush()
    // is the natural rate limiter (~25-30 fps).  No artificial frame pacing.
    while (1) {
        // Poll input (read current state only, edge snap done at end of frame)
        mm_input_poll();

        // ── Global hotkeys (BACK TO LAUNCHER) — C-side, works for ALL carts ──
        if (!s_is_launcher) {
            if (mm_input_boot_held()) {
                s_boot_hold++;
                if (s_boot_hold > 30) {  // ~1s hold
                    s_boot_hold = 0;
                    mm_lua_request_cart("/sdcard/system/launcher");
                    break;
                }
            } else {
                s_boot_hold = 0;
            }
            if (mm_input_select_pressed()) {
                s_boot_hold = 0;
                mm_lua_request_cart("/sdcard/system/launcher");
                break;
            }
        }

        // update()
        lua_getglobal(s_L, "update");
        if (lua_pcall(s_L, 0, 0, 0) != LUA_OK) {
            ESP_LOGE(TAG, "update() error: %s", lua_tostring(s_L, -1));
            lua_pop(s_L, 1);
            return ESP_FAIL;
        }

        // draw()
        lua_getglobal(s_L, "draw");
        if (lua_pcall(s_L, 0, 0, 0) != LUA_OK) {
            ESP_LOGE(TAG, "draw() error: %s", lua_tostring(s_L, -1));
            lua_pop(s_L, 1);
            return ESP_FAIL;
        }

        // Check if cart requested exit (system.exit() sets a flag)
        lua_getglobal(s_L, "__mm_exit_requested");
        bool exit_req = lua_toboolean(s_L, -1);
        lua_pop(s_L, 1);
        if (exit_req) {
            ESP_LOGI(TAG, "Cart exited cleanly");
            return ESP_OK;
        }

        // End-of-frame edge snapshot for input.pressed() detection
        mm_input_snap();

        // Yield to BLE/WiFi tasks (~10ms)
        vTaskDelay(1);
    }
    return ESP_OK;
}
