/*
 * key_test.cpp - "did the key fire?" diagnostic mode.
 *
 *   At boot:   shows reset reason for 1.5 s
 *   Then loop: shows last-pressed button label (BOOT key or UDP pad event)
 *              + duration since last event.
 *
 * Two input sources:
 *   1. GPIO0 BOOT button (polled every 40 ms)
 *   2. UDP pad packets on :8888 (via udp_pad component callback)
 *
 * Use this to physically/network-verify both LCD + input paths before
 * wiring the emulator back in.
 */

#include "key_test.h"
#include "display_bsp.h"
#include "udp_pad.h"

#include <esp_system.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <string.h>
#include <atomic>

namespace {

constexpr int W = 400;
constexpr int H = 300;
constexpr uint8_t BLACK = 0x00;
constexpr uint8_t WHITE = 0xff;
constexpr gpio_num_t BOOT_BTN = GPIO_NUM_0;
constexpr const char *TAG = "key_test";

/* 5x7 column-major font, bit0 = top row of column. */
struct Glyph { char ch; uint8_t col[5]; };
const Glyph kFont[] = {
    {' ', {0x00,0x00,0x00,0x00,0x00}},
    {'.', {0x00,0x60,0x60,0x00,0x00}},
    {'-', {0x08,0x08,0x08,0x08,0x08}},
    {'?', {0x02,0x01,0x51,0x09,0x06}},
    {'A', {0x7E,0x09,0x09,0x09,0x7E}},
    {'B', {0x7F,0x49,0x49,0x49,0x36}},
    {'C', {0x3E,0x41,0x41,0x41,0x22}},
    {'D', {0x7F,0x41,0x41,0x41,0x3E}},
    {'E', {0x7F,0x49,0x49,0x49,0x41}},
    {'F', {0x7F,0x09,0x09,0x09,0x01}},
    {'G', {0x3E,0x41,0x49,0x49,0x7A}},
    {'H', {0x7F,0x08,0x08,0x08,0x7F}},
    {'I', {0x00,0x41,0x7F,0x41,0x00}},
    {'K', {0x7F,0x08,0x14,0x22,0x41}},
    {'L', {0x7F,0x40,0x40,0x40,0x40}},
    {'M', {0x7F,0x02,0x0C,0x02,0x7F}},
    {'N', {0x7F,0x04,0x08,0x10,0x7F}},
    {'O', {0x3E,0x41,0x41,0x41,0x3E}},
    {'P', {0x7F,0x09,0x09,0x09,0x06}},
    {'R', {0x7F,0x09,0x19,0x29,0x46}},
    {'S', {0x46,0x49,0x49,0x49,0x31}},
    {'T', {0x01,0x01,0x7F,0x01,0x01}},
    {'U', {0x3F,0x40,0x40,0x40,0x3F}},
    {'W', {0x7F,0x20,0x18,0x20,0x7F}},
    {'Y', {0x07,0x08,0x70,0x08,0x07}},
};

const Glyph *find_glyph(char c)
{
    if (c >= 'a' && c <= 'z') c = (char)(c - 32);
    for (auto &g : kFont) if (g.ch == c) return &g;
    return &kFont[0];
}

inline void px(DisplayPort *lcd, int x, int y, uint8_t c)
{
    if ((unsigned)x < W && (unsigned)y < H) lcd->RLCD_SetPixel(x, y, c);
}

void fill(DisplayPort *lcd, int x0, int y0, int w, int h, uint8_t c)
{
    for (int y = y0; y < y0 + h; y++)
        for (int x = x0; x < x0 + w; x++) px(lcd, x, y, c);
}

int text_width(const char *s, int scale)
{
    int n = (int)strlen(s);
    return n > 0 ? n * 6 * scale - scale : 0;
}

void draw_text(DisplayPort *lcd, const char *s, int x0, int y0, int scale, uint8_t ink)
{
    int cx = x0;
    for (const char *p = s; *p; p++) {
        const Glyph *g = find_glyph(*p);
        for (int col = 0; col < 5; col++) {
            uint8_t bits = g->col[col];
            for (int row = 0; row < 7; row++) {
                if (bits & (1 << row))
                    fill(lcd, cx + col * scale, y0 + row * scale, scale, scale, ink);
            }
        }
        cx += 6 * scale;
    }
}

void draw_centered(DisplayPort *lcd, const char *big, const char *sub)
{
    fill(lcd, 0, 0, W, H, WHITE);

    int scale = 8;
    int tw = text_width(big, scale);
    if (tw > W - 16) { scale = 5; tw = text_width(big, scale); }
    if (tw > W - 16) { scale = 4; tw = text_width(big, scale); }
    int x = (W - tw) / 2;
    int y = (H - 7 * scale) / 2;
    draw_text(lcd, big, x, y, scale, BLACK);

    if (sub && *sub) {
        int sw = text_width(sub, 2);
        draw_text(lcd, sub, (W - sw) / 2, H - 28, 2, BLACK);
    }

    lcd->RLCD_Display();
}

const char *reset_reason_str(esp_reset_reason_t r)
{
    switch (r) {
        case ESP_RST_POWERON:   return "POWERON";
        case ESP_RST_EXT:       return "RST";
        case ESP_RST_SW:        return "SW";
        case ESP_RST_PANIC:     return "PANIC";
        case ESP_RST_INT_WDT:   return "INT WDT";
        case ESP_RST_TASK_WDT:  return "TASK WDT";
        case ESP_RST_WDT:       return "WDT";
        case ESP_RST_DEEPSLEEP: return "DEEPSLEEP";
        case ESP_RST_BROWNOUT:  return "BROWNOUT";
        case ESP_RST_SDIO:      return "SDIO";
        case ESP_RST_USB:       return "USB";
        default:                return "UNKNOWN";
    }
}

/* shared state between UDP callback (Core 0) and the render loop. */
struct LastInput {
    char        label[16];
    const char *state;       /* "PRESSED" / "RELEASED" */
    std::atomic<uint32_t> seq{0};   /* bumped on every event */
};
LastInput g_last;

void udp_cb(uint8_t btn_id, bool pressed, void * /*arg*/)
{
    const char *name = udp_pad_btn_name(btn_id);
    /* Copy name into static buffer (callback may be invoked again). */
    strncpy(g_last.label, name, sizeof(g_last.label) - 1);
    g_last.label[sizeof(g_last.label) - 1] = '\0';
    g_last.state = pressed ? "PRESSED" : "RELEASED";
    g_last.seq.fetch_add(1, std::memory_order_release);
}

}  /* anonymous namespace */

extern "C" void key_test_run(DisplayPort *lcd)
{
    gpio_config_t cfg = {};
    cfg.pin_bit_mask    = 1ULL << BOOT_BTN;
    cfg.mode            = GPIO_MODE_INPUT;
    cfg.pull_up_en      = GPIO_PULLUP_ENABLE;
    cfg.pull_down_en    = GPIO_PULLDOWN_DISABLE;
    cfg.intr_type       = GPIO_INTR_DISABLE;
    gpio_config(&cfg);

    /* 1. Splash with reset reason */
    const char *rr = reset_reason_str(esp_reset_reason());
    draw_centered(lcd, rr, "RESET REASON");
    vTaskDelay(pdMS_TO_TICKS(1500));

    /* 2. Subscribe to UDP pad events */
    udp_pad_set_callback(udp_cb, nullptr);

    /* 3. Idle screen */
    strcpy(g_last.label, "WAIT");
    g_last.state = "PRESS A KEY";
    draw_centered(lcd, g_last.label, g_last.state);
    uint32_t last_seq = g_last.seq.load(std::memory_order_acquire);

    int prev_boot = -2;

    while (true) {
        /* BOOT GPIO0 polling */
        int boot = (gpio_get_level(BOOT_BTN) == 0) ? 1 : 0;
        if (boot != prev_boot) {
            strcpy(g_last.label, "BOOT");
            g_last.state = boot ? "PRESSED" : "RELEASED";
            g_last.seq.fetch_add(1, std::memory_order_release);
            prev_boot = boot;
        }

        /* Redraw only when something changed (cheaper, avoids LCD flicker). */
        uint32_t cur = g_last.seq.load(std::memory_order_acquire);
        if (cur != last_seq) {
            ESP_LOGI(TAG, "redraw: %s %s", g_last.label, g_last.state);
            draw_centered(lcd, g_last.label, g_last.state);
            last_seq = cur;
        }
        vTaskDelay(pdMS_TO_TICKS(30));
    }
}
