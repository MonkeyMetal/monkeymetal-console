/// @file gfx_framebuffer.c
/// @brief 16bpp framebuffer allocation and management

#include "gfx_engine.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "gfx_fb";

// 16bpp framebuffer: 400×300×2 = 240,000 bytes
EXT_RAM_BSS_ATTR static uint16_t s_framebuffer[GFX_WIDTH * GFX_HEIGHT];

esp_err_t gfx_init(void)
{
    ESP_LOGI(TAG, "Initializing graphics engine");
    ESP_LOGI(TAG, "Framebuffer: %dx%d 16bpp = %zu KB in PSRAM",
             GFX_WIDTH, GFX_HEIGHT, sizeof(s_framebuffer) / 1024);

    // Clear to white (reflective LCD default)
    gfx_clear(GFX_WHITE);

    return ESP_OK;
}

void gfx_clear(gfx_color_t color)
{
    for (int i = 0; i < GFX_WIDTH * GFX_HEIGHT; i++) {
        s_framebuffer[i] = color;
    }
}

void gfx_pixel(int x, int y, gfx_color_t color)
{
    if (x < 0 || x >= GFX_WIDTH || y < 0 || y >= GFX_HEIGHT) {
        return; // Clip
    }
    s_framebuffer[y * GFX_WIDTH + x] = color;
}

// Internal: get pixel (for circle fill)
static inline gfx_color_t gfx_get_pixel(int x, int y)
{
    if (x < 0 || x >= GFX_WIDTH || y < 0 || y >= GFX_HEIGHT) {
        return GFX_BLACK;
    }
    return s_framebuffer[y * GFX_WIDTH + x];
}

// Expose framebuffer pointer for Bayer dither
uint16_t *gfx_get_framebuffer(void)
{
    return s_framebuffer;
}
