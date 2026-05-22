/*
 * udp_pad.c - UDP keypad listener task.
 *
 * One task pinned to Core 0 (so it doesn't fight the emulator on Core 1):
 *   socket(SOCK_DGRAM) -> bind(0.0.0.0:8888) -> recv loop
 *   For each 2-byte packet, decode and forward to:
 *     - test callback (if installed)  -- used by key_test mode
 *     - gnuboy_sys_post_key()         -- when gnuboy is wired in
 *
 * gnuboy keymap (from sys/sdl/keymap.c defaults):
 *   K_UP/K_DOWN/K_LEFT/K_RIGHT for the dpad,
 *   'a' / 'b' for A and B,
 *   K_ENTER ('\r') for START, K_TAB ('\t') for SELECT.
 */

#include "udp_pad.h"

#include <stdbool.h>
#include <string.h>
#include <errno.h>

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <lwip/sockets.h>
#include <lwip/netdb.h>

#include "gnuboy_sys_esp32idf.h"

/* gnuboy input.h key codes (used by gnuboy_sys_post_key) */
#define K_UP    0x10a
#define K_DOWN  0x10b
#define K_RIGHT 0x10c
#define K_LEFT  0x10d
#define K_ENTER '\r'
#define K_TAB   '\t'

#define UDP_PAD_PORT 8888

static const char *TAG = "udp_pad";
static udp_pad_cb_t s_cb = NULL;
static void        *s_cb_arg = NULL;

static int btn_to_gnuboy(uint8_t btn)
{
    switch (btn) {
        case UDP_PAD_UP:     return K_UP;
        case UDP_PAD_DOWN:   return K_DOWN;
        case UDP_PAD_LEFT:   return K_LEFT;
        case UDP_PAD_RIGHT:  return K_RIGHT;
        case UDP_PAD_A:      return 'a';
        case UDP_PAD_B:      return 'b';
        case UDP_PAD_START:  return K_ENTER;
        case UDP_PAD_SELECT: return K_TAB;
        default:             return -1;
    }
}

const char *udp_pad_btn_name(uint8_t btn)
{
    switch (btn) {
        case UDP_PAD_UP:     return "UP";
        case UDP_PAD_DOWN:   return "DOWN";
        case UDP_PAD_LEFT:   return "LEFT";
        case UDP_PAD_RIGHT:  return "RIGHT";
        case UDP_PAD_A:      return "A";
        case UDP_PAD_B:      return "B";
        case UDP_PAD_START:  return "START";
        case UDP_PAD_SELECT: return "SELECT";
        default:             return "?";
    }
}

void udp_pad_set_callback(udp_pad_cb_t cb, void *arg)
{
    s_cb = cb;
    s_cb_arg = arg;
}

static void udp_pad_task(void *param)
{
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock < 0) {
        ESP_LOGE(TAG, "socket() failed errno=%d", errno);
        vTaskDelete(NULL);
        return;
    }

    struct sockaddr_in addr = {};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port        = htons(UDP_PAD_PORT);
    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        ESP_LOGE(TAG, "bind(:%d) failed errno=%d", UDP_PAD_PORT, errno);
        close(sock);
        vTaskDelete(NULL);
        return;
    }
    ESP_LOGI(TAG, "listening on UDP :%d", UDP_PAD_PORT);

    uint8_t buf[16];
    struct sockaddr_in src = {};
    socklen_t sl = sizeof(src);
    while (true) {
        int n = recvfrom(sock, buf, sizeof(buf), 0,
                        (struct sockaddr *)&src, &sl);
        if (n < 0) {
            if (errno == EINTR) continue;
            ESP_LOGW(TAG, "recv errno=%d, sleeping 200ms", errno);
            vTaskDelay(pdMS_TO_TICKS(200));
            continue;
        }
        if (n < 2) continue;     /* runt packet, ignore */

        uint8_t btn  = buf[0];
        bool pressed = (buf[1] != 0);
        if (btn < 1 || btn > UDP_PAD_MAX) continue;

        ESP_LOGI(TAG, "%-6s %s", udp_pad_btn_name(btn),
                 pressed ? "DOWN" : "UP");

        /* Always forward to gnuboy queue (no-op if queue not yet ready). */
        int gk = btn_to_gnuboy(btn);
        if (gk >= 0) gnuboy_sys_post_key(gk, pressed);

        /* Diagnostic / test hook. */
        if (s_cb) s_cb(btn, pressed, s_cb_arg);
    }
}

void udp_pad_init(void)
{
    /* 4 KB stack is enough for lwip socket calls; pin to Core 0
     * so it doesn't compete with the emu thread on Core 1. */
    xTaskCreatePinnedToCore(udp_pad_task, "udp_pad",
                            4096, NULL, 5, NULL, 0);
}
