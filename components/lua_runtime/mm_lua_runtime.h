/// @file mm_lua_runtime.h
/// @brief MonkeyMetal Lua runtime — public API

#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/// @brief Initialize Lua VM with PSRAM allocator and register all bindings.
/// @return ESP_OK on success
esp_err_t mm_lua_init(void);

/// @brief Load and execute a cart from SD card.
/// @param cart_dir  Path to cart directory, e.g. "/sdcard/games/snake"
///                  Must contain cart.lua. Calls init() then runs update()/draw() loop.
/// @return ESP_OK if cart exited cleanly, ESP_FAIL on Lua error
esp_err_t mm_lua_run_cart(const char *cart_dir);

/// @brief Deinitialize Lua VM and free PSRAM.
void mm_lua_deinit(void);

#ifdef __cplusplus
}
#endif
