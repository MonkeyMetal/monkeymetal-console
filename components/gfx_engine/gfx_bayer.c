/// @file gfx_bayer.c
/// @brief Bayer 4×4 ordered dithering: 16bpp → 1bpp
///
/// ST7305 landscape (400×300) pixel packing:
///   Each byte covers a 2-column × 4-row block.
///   byte index = (x/2) * (HEIGHT/4) + (inv_y/4)
///   bit  index = 7 - ((inv_y%4)*2 + x%2)
///   where inv_y = HEIGHT-1-y  (ST7305 scans bottom-to-top)

#include "gfx_engine.h"
#include "esp_log.h"
#include "port_bsp.h"
#include <string.h>

static const char *TAG = "gfx_bayer";

// Bayer 4×4 threshold matrix (scaled to 0–255)
static const uint8_t bayer_matrix[4][4] = {
    {  0, 128,  32, 160 },
    { 192,  64, 224,  96 },
    {  48, 176,  16, 144 },
    { 240, 112, 208,  80 }
};

// 1bpp output buffer: 400×300 / 8 = 15,000 bytes
EXT_RAM_BSS_ATTR static uint8_t s_output_1bpp[GFX_WIDTH * GFX_HEIGHT / 8];

// External: get framebuffer pointer
extern uint16_t *gfx_get_framebuffer(void);

esp_err_t gfx_flush(void)
{
    uint16_t *fb = gfx_get_framebuffer();
    memset(s_output_1bpp, 0, sizeof(s_output_1bpp));

    // ST7305 landscape geometry constants
    const int H4 = GFX_HEIGHT / 4;  // 75

    for (int y = 0; y < GFX_HEIGHT; y++) {
        int inv_y   = GFX_HEIGHT - 1 - y;
        int block_y = inv_y / 4;
        int local_y = inv_y % 4;

        for (int x = 0; x < GFX_WIDTH; x++) {
            uint16_t gray16 = fb[y * GFX_WIDTH + x];
            uint8_t  gray8  = gray16 >> 8;

            // Bayer threshold
            uint8_t threshold = bayer_matrix[y & 3][x & 3];
            if (gray8 <= threshold) {
                continue; // pixel = 0, buffer already cleared
            }

            // Pack into ST7305 landscape 1bpp format
            int byte_x  = x / 2;
            int local_x = x % 2;
            uint32_t idx = (uint32_t)byte_x * H4 + block_y;
            uint8_t  bit = 7 - (local_y * 2 + local_x);
            s_output_1bpp[idx] |= (1u << bit);
        }
    }

    esp_err_t ret = port_bsp_lcd_flush(s_output_1bpp, sizeof(s_output_1bpp));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LCD flush failed: %s", esp_err_to_name(ret));
    }

    return ret;
}
