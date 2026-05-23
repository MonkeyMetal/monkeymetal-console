/// @file gfx_draw.c
/// @brief Basic drawing primitives (line, rect, circle)

#include "gfx_engine.h"
#include <stdlib.h>
#include <math.h>

// Bresenham line
void gfx_line(int x1, int y1, int x2, int y2, gfx_color_t color)
{
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;

    while (true) {
        gfx_pixel(x1, y1, color);
        if (x1 == x2 && y1 == y2) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x1 += sx; }
        if (e2 < dx)  { err += dx; y1 += sy; }
    }
}

void gfx_rect(int x, int y, int w, int h, gfx_color_t color, bool fill)
{
    if (fill) {
        for (int dy = 0; dy < h; dy++) {
            for (int dx = 0; dx < w; dx++) {
                gfx_pixel(x + dx, y + dy, color);
            }
        }
    } else {
        // Outline
        gfx_line(x, y, x + w - 1, y, color);           // top
        gfx_line(x, y + h - 1, x + w - 1, y + h - 1, color); // bottom
        gfx_line(x, y, x, y + h - 1, color);           // left
        gfx_line(x + w - 1, y, x + w - 1, y + h - 1, color); // right
    }
}

// Midpoint circle (outline)
static void circle_outline(int cx, int cy, int r, gfx_color_t color)
{
    int x = r, y = 0;
    int err = 0;

    while (x >= y) {
        gfx_pixel(cx + x, cy + y, color);
        gfx_pixel(cx + y, cy + x, color);
        gfx_pixel(cx - y, cy + x, color);
        gfx_pixel(cx - x, cy + y, color);
        gfx_pixel(cx - x, cy - y, color);
        gfx_pixel(cx - y, cy - x, color);
        gfx_pixel(cx + y, cy - x, color);
        gfx_pixel(cx + x, cy - y, color);

        if (err <= 0) {
            y++;
            err += 2 * y + 1;
        }
        if (err > 0) {
            x--;
            err -= 2 * x + 1;
        }
    }
}

// Filled circle (scanline fill)
static void circle_fill(int cx, int cy, int r, gfx_color_t color)
{
    for (int y = -r; y <= r; y++) {
        int x = (int)(0.5 + sqrt(r * r - y * y));
        for (int dx = -x; dx <= x; dx++) {
            gfx_pixel(cx + dx, cy + y, color);
        }
    }
}

void gfx_circle(int cx, int cy, int r, gfx_color_t color, bool fill)
{
    if (fill) {
        circle_fill(cx, cy, r, color);
    } else {
        circle_outline(cx, cy, r, color);
    }
}
