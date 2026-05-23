/// @file gfx_engine.h
/// @brief MonkeyMetal Console Graphics Engine - M1
///
/// 16bpp framebuffer (400×300×2 = 240 KB in PSRAM)
/// → Bayer 4×4 dither → 1bpp output (15 KB)
/// → ST7305 LCD via port_bsp

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// Display dimensions (ST7305 physical)
#define GFX_WIDTH  400
#define GFX_HEIGHT 300

// Color representation (16bpp grayscale: 0=black, 65535=white)
typedef uint16_t gfx_color_t;

#define GFX_BLACK  0x0000
#define GFX_WHITE  0xFFFF
#define GFX_GRAY(x) ((uint16_t)((x) * 257))  // 0–255 → 0–65535

/// @brief Initialize graphics engine. Allocates PSRAM framebuffer.
/// @return ESP_OK on success
esp_err_t gfx_init(void);

/// @brief Clear framebuffer with given color
/// @param color 16bpp grayscale (0=black, 65535=white)
void gfx_clear(gfx_color_t color);

/// @brief Set single pixel
/// @param x, y Coordinates (clipped to screen bounds)
/// @param color 16bpp grayscale
void gfx_pixel(int x, int y, gfx_color_t color);

/// @brief Draw line (Bresenham)
/// @param x1, y1, x2, y2 Endpoints
/// @param color 16bpp grayscale
void gfx_line(int x1, int y1, int x2, int y2, gfx_color_t color);

/// @brief Draw rectangle
/// @param x, y Top-left corner
/// @param w, h Width and height
/// @param color 16bpp grayscale
/// @param fill true=filled, false=outline only
void gfx_rect(int x, int y, int w, int h, gfx_color_t color, bool fill);

/// @brief Draw circle (midpoint algorithm)
/// @param cx, cy Center
/// @param r Radius
/// @param color 16bpp grayscale
/// @param fill true=filled, false=outline only
void gfx_circle(int cx, int cy, int r, gfx_color_t color, bool fill);

/// @brief Apply Bayer 4×4 dither and push to LCD. Blocks until DMA completes (~30 ms).
/// @return ESP_OK on success
esp_err_t gfx_flush(void);

#ifdef __cplusplus
}
#endif
