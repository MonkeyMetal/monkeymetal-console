/// @file port_bsp.h
/// @brief C interface for port_bsp (LCD, SD, etc.)

#pragma once

#include "esp_err.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/// @brief Push 1bpp framebuffer to ST7305 LCD. Blocks until DMA completes.
/// @param buffer 1bpp data, 400×300/8 = 15,000 bytes
/// @param len Buffer length in bytes
/// @return ESP_OK on success
esp_err_t port_bsp_lcd_flush(const uint8_t *buffer, size_t len);

#ifdef __cplusplus
}
#endif
