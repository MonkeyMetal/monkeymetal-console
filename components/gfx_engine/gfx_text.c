/// @file gfx_text.c
/// @brief 8×8 bitmap font text rendering with bold (anti-dither)

#include "gfx_engine.h"
#include "gfx_font8x8.h"

void gfx_text_char(char ch, int x, int y, gfx_color_t color)
{
    if (ch < 32 || ch > 126) ch = '?';
    const uint8_t *glyph = gfx_font8x8[(uint8_t)ch - 32];

    for (int row = 0; row < 8; row++) {
        uint8_t bits = glyph[row];
        for (int col = 0; col < 8; col++) {
            if (bits & (0x80 >> col)) {
                gfx_pixel(x + col, y + row, color);
            }
        }
    }
}

void gfx_text(const char *s, int x, int y, gfx_color_t color)
{
    int cx = x;
    while (*s) {
        if (*s == '\n') {
            cx = x;
            y += 10;
        } else {
            // Draw regular, then again shifted right 1px for bold
            gfx_text_char(*s, cx,     y, color);
            gfx_text_char(*s, cx + 1, y, color);
            cx += 8;
        }
        s++;
    }
}
