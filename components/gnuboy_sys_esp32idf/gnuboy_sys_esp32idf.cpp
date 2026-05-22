/*
 * gnuboy_sys_esp32idf.cpp - gnuboy platform port for ESP32-S3-RLCD-4.2
 *
 * Implements every hook declared in gnuboy/sys.h so gnuboy core links cleanly:
 *   vid_init/preinit/begin/end/close/setpal/settitle  display
 *   pcm_init/submit/close/pause                       audio (stub)
 *   ev_poll                                           event bridge
 *   sys_init/sleep/elapsed/timer/checkdir/sanitize    services
 *   joy_x and kb_x                                    empty stubs
 *
 * Framebuffer: 160x144 16bpp in PSRAM (~46 KiB). vid_end runs dither + push.
 * Stage-1 dither is the simplest threshold + center on screen (no scaling).
 * Task 16 will replace it with Bayer 4x4 + async push task.
 */

#include "gnuboy_sys_esp32idf.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/time.h>
#include <esp_log.h>
#include <esp_timer.h>
#include <esp_heap_caps.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

/* gnuboy headers - gnuboy core is C, link with C linkage. */
extern "C" {
#include "defs.h"
#include "fb.h"
#include "pcm.h"
#include "input.h"
#include "loader.h"
#include "emu.h"
#include "rc.h"
}

/* port_bsp - ST7305 LCD driver (C++ class) */
#include "display_bsp.h"

static const char *TAG = "sys_esp32idf";

/* main.cpp owns the LCD instance */
extern DisplayPort g_lcd;

/* ============================================================
 * gnuboy globals (declared extern in fb.h / pcm.h)
 * Need C linkage so the C core can resolve them.
 * ============================================================ */
extern "C" {
struct fb fb;
struct pcm pcm;
}

/* ============================================================
 * GameBoy 16bpp framebuffer in PSRAM
 * ============================================================ */
#define GB_W 160
#define GB_H 144
static uint16_t *s_gb_fb = NULL;     /* GB_W*GB_H, RGB565 */

/* Palette LUT - vid_setpal feeds it (GBC palettes change per frame).
 * Currently unused since the dither path reads RGB565 from fb directly. */
static uint16_t s_pal16[64];

/* ============================================================
 * Input event queue (UDP task / hardware buttons feed it)
 * ============================================================ */
typedef struct { int code; bool pressed; } sys_input_evt_t;
static QueueHandle_t s_input_q = NULL;

/* ============================================================
 * Frame timing baseline
 * ============================================================ */
static int64_t s_timer_us = 0;

/* ============================================================
 * vid_* display hooks
 * ============================================================ */
extern "C" void vid_preinit(void) {}

extern "C" void vid_init(void)
{
    s_gb_fb = (uint16_t *)heap_caps_malloc(GB_W * GB_H * 2, MALLOC_CAP_SPIRAM);
    if (!s_gb_fb) { ESP_LOGE(TAG, "PSRAM alloc fb FAILED"); abort(); }
    memset(s_gb_fb, 0, GB_W * GB_H * 2);

    fb.ptr     = (byte *)s_gb_fb;
    fb.w       = GB_W;
    fb.h       = GB_H;
    fb.pelsize = 2;                  /* 16 bpp */
    fb.pitch   = GB_W * 2;
    fb.indexed = 0;
    fb.yuv     = 0;
    fb.enabled = 1;
    fb.dirty   = 0;
    /* RGB565 little-endian: R(5) G(6) B(5) */
    fb.cc[0].l = 11; fb.cc[0].r = 3;     /* red:    >>3, <<11 */
    fb.cc[1].l = 5;  fb.cc[1].r = 2;     /* green:  >>2, <<5  */
    fb.cc[2].l = 0;  fb.cc[2].r = 3;     /* blue:   >>3, <<0  */
    fb.cc[3].l = 0;  fb.cc[3].r = 0;     /* alpha:  unused    */

    ESP_LOGI(TAG, "vid_init: GB fb %dx%d @16bpp = %d bytes in PSRAM",
             GB_W, GB_H, GB_W * GB_H * 2);
}

extern "C" void vid_close(void)
{
    if (s_gb_fb) { heap_caps_free(s_gb_fb); s_gb_fb = NULL; }
    fb.enabled = 0;
}

extern "C" void vid_settitle(char *title)
{
    ESP_LOGI(TAG, "ROM title: %s", title ? title : "(null)");
}

extern "C" void vid_setpal(int i, int r, int g, int b)
{
    if (i < 0 || i >= (int)(sizeof(s_pal16)/sizeof(s_pal16[0]))) return;
    /* gnuboy passes 0..255 channels, encode to RGB565 */
    uint16_t v = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
    s_pal16[i] = v;
}

extern "C" void vid_begin(void)
{
    fb.dirty = 0;
}

/* vid_end: synchronous dither + push. Task #16 will make it async. */
extern "C" void vid_end(void)
{
    if (!s_gb_fb) return;

    /* Center 160x144 GB area into 400x300 panel: top-left at (120, 78). */
    const int ox = (400 - GB_W) / 2;          /* 120 */
    const int oy = (300 - GB_H) / 2;          /* 78  */
    const uint16_t *p = s_gb_fb;
    for (int y = 0; y < GB_H; y++) {
        for (int x = 0; x < GB_W; x++) {
            uint16_t c = *p++;
            /* Threshold on weighted RGB565 luma: R*0.30 + G*0.59 + B*0.11 */
            int r = (c >> 11) & 0x1F;
            int g = (c >> 5)  & 0x3F;
            int b = (c)       & 0x1F;
            int luma = (r * 77 + (g >> 1) * 150 + b * 28) >> 8;  /* ~0..31 */
            int color = (luma > 12) ? 0xff : 0x00;   /* 0xff white, 0x00 black */
            g_lcd.RLCD_SetPixel(ox + x, oy + y, (uint8_t)color);
        }
    }
    g_lcd.RLCD_Display();
}

/* ============================================================
 * pcm_* audio - stage-1 stubs (silent)
 * ============================================================ */
extern "C" void pcm_init(void)
{
    pcm.hz     = 16000;
    pcm.stereo = 0;
    pcm.len    = 0;
    pcm.buf    = NULL;
    pcm.pos    = 0;
}

extern "C" int pcm_submit(void)         { return 0; } /* 0 -> emu_run won't block on us */
extern "C" void pcm_close(void)         {}
extern "C" void pcm_pause(int dopause)  { (void)dopause; }

/* ============================================================
 * ev_poll - bridge FreeRTOS queue -> gnuboy ev_postevent
 * ============================================================ */
extern "C" void ev_poll(int wait)
{
    (void)wait;
    if (!s_input_q) return;
    sys_input_evt_t e;
    while (xQueueReceive(s_input_q, &e, 0) == pdTRUE) {
        event_t gev;
        memset(&gev, 0, sizeof(gev));
        gev.type = e.pressed ? EV_PRESS : EV_RELEASE;
        gev.code = e.code;
        ev_postevent(&gev);
    }
}

extern "C" void gnuboy_sys_post_key(int gnuboy_keycode, bool pressed)
{
    if (!s_input_q) return;
    sys_input_evt_t e = { gnuboy_keycode, pressed };
    xQueueSend(s_input_q, &e, 0);
}

/* ============================================================
 * sys_* services
 * ============================================================ */
extern "C" void *sys_timer(void)
{
    s_timer_us = esp_timer_get_time();
    return &s_timer_us;
}

extern "C" int sys_elapsed(struct timeval *prev_void)
{
    int64_t *prev = (int64_t *)prev_void;
    int64_t now = esp_timer_get_time();
    int64_t dt  = now - *prev;
    *prev = now;
    return (int)dt;          /* gnuboy expects microseconds */
}

extern "C" void sys_sleep(int us)
{
    if (us <= 0) return;
    if (us < 1000) {
        /* < 1ms: at least one scheduler tick (1ms @1kHz) so we yield */
        vTaskDelay(1);
    } else {
        vTaskDelay(pdMS_TO_TICKS(us / 1000));
    }
}

extern "C" void sys_checkdir(char *path, int wr) { (void)path; (void)wr; }
extern "C" void sys_sanitize(char *s)            { (void)s; }
extern "C" void sys_initpath(void)               {}

/* ============================================================
 * joy_x and kb_x device hooks - we feed events via ev_poll, these stay empty
 * ============================================================ */
extern "C" void joy_init(void)  {}
extern "C" void joy_poll(void)  {}
extern "C" void joy_close(void) {}
extern "C" void kb_init(void)   {}
extern "C" void kb_poll(void)   {}
extern "C" void kb_close(void)  {}

/* ============================================================
 * doevents() - originally in gnuboy main.c (PC entry). Reimplemented
 * here so emu.c (which calls doevents() inside emu_run) links cleanly.
 * Pulls events from our queue via ev_poll, then forwards key events
 * into gnuboy's rc_dokey for game pad action.
 * ============================================================ */
extern "C" void rc_dokey(int key, int st);   /* declared in rc.h */

extern "C" void doevents(void)
{
    event_t ev;
    ev_poll(0);
    while (ev_getevent(&ev)) {
        if (ev.type != EV_PRESS && ev.type != EV_RELEASE) continue;
        int st = (ev.type != EV_RELEASE);
        rc_dokey(ev.code, st);
    }
}

/* ============================================================
 * die() - gnuboy unrecoverable panic. Declared in defs.h, sys impl.
 * ============================================================ */
extern "C" void die(char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    char buf[160];
    vsnprintf(buf, sizeof(buf), fmt ? fmt : "(null)", ap);
    va_end(ap);
    ESP_LOGE(TAG, "GNUBOY DIE: %s", buf);
    abort();
}

/* ============================================================
 * Public API
 * ============================================================ */
void gnuboy_sys_init(void)
{
    s_input_q = xQueueCreate(32, sizeof(sys_input_evt_t));
    ESP_LOGI(TAG, "sys_init: input queue ready (cap=32)");
}

void gnuboy_sys_run(const char *rom_path)
{
    ESP_LOGI(TAG, "loader_init(\"%s\")", rom_path);

    /* gnuboy expects char*, loader_init copies internally. */
    if (loader_init((char *)rom_path) < 0) {
        ESP_LOGE(TAG, "loader_init failed: %s", loader_get_error());
        ESP_LOGE(TAG, "  put rom.gbc on the SD card root and power-cycle.");
        while (1) vTaskDelay(pdMS_TO_TICKS(5000));
    }

    ESP_LOGI(TAG, "emu_reset + emu_run");
    emu_reset();
    emu_run();   /* never returns */
}
