/*
 * monkey_metal.cpp - "MonkeyMetal" boot splash.
 *
 *  ascii blueprint of the monkey (banana for scale):
 *
 *        ___       ___
 *       /   \_____/   \         <- ears
 *      |  o   ___   o  |        <- eyes (blink)
 *      |    /     \    |
 *      |   |  ___  |   |        <- muzzle
 *       \   \___/   /
 *        \_________/
 *
 *  Below it, the word METAL slides in from the right.
 */

#include "monkey_metal.h"
#include "display_bsp.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <string.h>

namespace {

constexpr int W = 400;
constexpr int H = 300;
constexpr uint8_t BLACK = 0x00;
constexpr uint8_t WHITE = 0xff;

inline void px(DisplayPort *lcd, int x, int y, uint8_t c)
{
    if ((unsigned)x >= W || (unsigned)y >= H) return;
    lcd->RLCD_SetPixel(x, y, c);
}

void fill_rect(DisplayPort *lcd, int x0, int y0, int w, int h, uint8_t c)
{
    for (int y = y0; y < y0 + h; y++)
        for (int x = x0; x < x0 + w; x++) px(lcd, x, y, c);
}

/* Filled axis-aligned ellipse (Bresenham). */
void fill_ellipse(DisplayPort *lcd, int cx, int cy, int rx, int ry, uint8_t c)
{
    long rx2 = (long)rx * rx, ry2 = (long)ry * ry;
    for (int y = -ry; y <= ry; y++) {
        long y2 = (long)y * y;
        for (int x = -rx; x <= rx; x++) {
            if ((long)x * x * ry2 + y2 * rx2 <= rx2 * ry2)
                px(lcd, cx + x, cy + y, c);
        }
    }
}

/* Hollow ellipse outline (1px). */
void stroke_ellipse(DisplayPort *lcd, int cx, int cy, int rx, int ry, uint8_t c)
{
    long rx2 = (long)rx * rx, ry2 = (long)ry * ry;
    /* Two-pass scan: shell = filled minus inner. Easier than Bresenham edge. */
    for (int y = -ry; y <= ry; y++) {
        long y2 = (long)y * y;
        int prev_inside = 0;
        for (int x = -rx; x <= rx; x++) {
            int inside = ((long)x * x * ry2 + y2 * rx2 <= rx2 * ry2);
            if (inside != prev_inside) px(lcd, cx + x, cy + y, c);
            prev_inside = inside;
        }
        if (prev_inside) px(lcd, cx + rx, cy + y, c);
    }
}

/* ------- 5x7 mini font, just A-Z plus space, packed column-major -------
 *  Each char is 5 cols, 7 rows. Bit 0 = top row of that column.            */
struct Glyph { char ch; uint8_t col[5]; };
const Glyph kFont[] = {
    {' ', {0x00,0x00,0x00,0x00,0x00}},
    {'M', {0x7F,0x02,0x0C,0x02,0x7F}},
    {'O', {0x3E,0x41,0x41,0x41,0x3E}},
    {'N', {0x7F,0x04,0x08,0x10,0x7F}},
    {'K', {0x7F,0x08,0x14,0x22,0x41}},
    {'E', {0x7F,0x49,0x49,0x49,0x41}},
    {'Y', {0x07,0x08,0x70,0x08,0x07}},
    {'T', {0x01,0x01,0x7F,0x01,0x01}},
    {'A', {0x7E,0x09,0x09,0x09,0x7E}},
    {'L', {0x7F,0x40,0x40,0x40,0x40}},
};

const Glyph *find_glyph(char c)
{
    for (auto &g : kFont) if (g.ch == c) return &g;
    return &kFont[0];
}

/* Draw text at (x,y) with per-pixel scale 's'. Color = ink, bg untouched. */
void draw_text(DisplayPort *lcd, const char *s, int x0, int y0, int scale, uint8_t ink)
{
    int cx = x0;
    for (const char *p = s; *p; p++) {
        const Glyph *g = find_glyph(*p);
        for (int col = 0; col < 5; col++) {
            uint8_t bits = g->col[col];
            for (int row = 0; row < 7; row++) {
                if (bits & (1 << row)) {
                    fill_rect(lcd, cx + col * scale, y0 + row * scale, scale, scale, ink);
                }
            }
        }
        cx += 6 * scale; /* 5 cols + 1 space */
    }
}

/* Ape head silhouette + features.  All hard-coded for 400x300 panel. */
void draw_monkey(DisplayPort *lcd, bool eyes_open)
{
    const int cx = 200, cy = 130;

    /* clear background to white */
    fill_rect(lcd, 0, 0, W, H, WHITE);

    /* outer ears */
    fill_ellipse(lcd, cx - 70, cy, 22, 26, BLACK);
    fill_ellipse(lcd, cx + 70, cy, 22, 26, BLACK);
    /* inner ear highlight */
    fill_ellipse(lcd, cx - 70, cy, 12, 15, WHITE);
    fill_ellipse(lcd, cx + 70, cy, 12, 15, WHITE);

    /* head */
    fill_ellipse(lcd, cx, cy, 65, 60, BLACK);

    /* face mask */
    fill_ellipse(lcd, cx, cy + 8, 45, 42, WHITE);

    /* eyes */
    if (eyes_open) {
        fill_ellipse(lcd, cx - 18, cy - 5, 7, 9, BLACK);
        fill_ellipse(lcd, cx + 18, cy - 5, 7, 9, BLACK);
        /* pupils glint */
        fill_ellipse(lcd, cx - 16, cy - 7, 2, 2, WHITE);
        fill_ellipse(lcd, cx + 20, cy - 7, 2, 2, WHITE);
    } else {
        /* blink: simple horizontal slits */
        fill_rect(lcd, cx - 25, cy - 5, 14, 2, BLACK);
        fill_rect(lcd, cx + 11, cy - 5, 14, 2, BLACK);
    }

    /* nose: two nostrils */
    fill_ellipse(lcd, cx - 5, cy + 14, 2, 3, BLACK);
    fill_ellipse(lcd, cx + 5, cy + 14, 2, 3, BLACK);

    /* mouth: friendly arc - render as ellipse outline lower half */
    for (int x = -16; x <= 16; x++) {
        int y = (int)((float)x * x / 16.0f - 14); /* parabola */
        px(lcd, cx + x, cy + 28 + y, BLACK);
        px(lcd, cx + x, cy + 28 + y + 1, BLACK);
    }
}

}  /* anonymous namespace */

extern "C" void monkey_metal_play(DisplayPort *lcd)
{
    /* Frame 1: monkey, eyes open, no banner */
    draw_monkey(lcd, true);
    draw_text(lcd, "MONKEY METAL", 116, 220, 4, BLACK);
    lcd->RLCD_Display();
    vTaskDelay(pdMS_TO_TICKS(600));

    /* Frame 2: blink */
    draw_monkey(lcd, false);
    draw_text(lcd, "MONKEY METAL", 116, 220, 4, BLACK);
    lcd->RLCD_Display();
    vTaskDelay(pdMS_TO_TICKS(150));

    /* Frame 3: eyes open again, second beat */
    draw_monkey(lcd, true);
    draw_text(lcd, "MONKEY METAL", 116, 220, 4, BLACK);
    /* underline */
    fill_rect(lcd, 116, 250, 12 * 6 * 4 - 4, 3, BLACK);
    lcd->RLCD_Display();
    vTaskDelay(pdMS_TO_TICKS(900));
}
