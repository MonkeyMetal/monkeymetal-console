/*
 * MonkeyMetal Console - main.cpp (M6: Launcher)
 *
 * Boot sequence:
 *   1. NVS init
 *   2. LCD bring-up + splash
 *   3. SD mount
 *   4. Wi-Fi STA
 *   5. Graphics engine
 *   6. Audio engine
 *   7. BT HID host (reconnect bonded or scan)
 *   8. Launcher loop: /sdcard/system/launcher → games → back
 */

#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>

#include "user_config.h"
#include "display_bsp.h"
#include "sdcard_bsp.h"
#include "wifi_bsp.h"
#include "wifi_file_server.h"
#include "monkey_metal.h"
#include "gfx_engine.h"
#include "mm_lua_runtime.h"
#include "audio_engine.h"
#include "bt_hid_host.h"
#include "esp_sntp.h"
#include "driver/gpio.h"

static const char *TAG = "mm";

/* Global LCD instance - referenced by port_bsp and other subsystems via extern */
DisplayPort g_lcd(GBEMU_LCD_MOSI_GPIO, GBEMU_LCD_SCK_GPIO,
                  GBEMU_LCD_DC_GPIO, GBEMU_LCD_CS_GPIO, GBEMU_LCD_RST_GPIO,
                  GBEMU_LCD_W, GBEMU_LCD_H);

static CustomSDPort *g_sd = nullptr;

// ── BT Pairing Screen ───────────────────────────────────────────────────────
// Simple 8×8 font UI. BOOT key controls selection.
#define BOOT_GPIO  GPIO_NUM_0

static bool boot_pressed(void)
{
    return gpio_get_level(BOOT_GPIO) == 0;  // active-low with pullup
}

static void draw_centered(const char *s, int y, gfx_color_t c)
{
    int w_px = (int)strlen(s) * 8;
    int x = (GFX_WIDTH - w_px) / 2;
    if (x < 0) x = 0;
    gfx_text(s, x, y, c);
}

static void bt_pairing_screen(void)
{
    gpio_config_t boot_cfg = {
        .pin_bit_mask = (1ULL << BOOT_GPIO),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&boot_cfg);

    int  wait_elapsed   = 0;
    int  last_flush_ms   = 0;
    bool prev_boot       = false;

    while (wait_elapsed < 15000) {
        vTaskDelay(pdMS_TO_TICKS(200));
        wait_elapsed += 200;

        // BOOT short-press = skip (exit pairing)
        bool cur_boot = boot_pressed();
        if (!cur_boot && prev_boot) {
            // Released after press → skip
            bt_hid_scan_stop();
            ESP_LOGI(TAG, "BT pairing skipped by user");
            return;
        }
        prev_boot = cur_boot;

        bt_hid_scan_result_t devs[8];
        int ndev = bt_hid_get_scan_results(devs, 8);

        // Auto-connect: exactly 1 device found → connect immediately
        if (ndev == 1) {
            bt_hid_scan_stop();

            gfx_clear(GFX_WHITE);
            draw_centered("CONNECTING...", GFX_HEIGHT / 2 - 24, GFX_BLACK);
            gfx_text(devs[0].name, 40, GFX_HEIGHT / 2, GFX_GRAY(128));
            gfx_flush();

            esp_err_t ret = bt_hid_connect_ex(devs[0].bda, devs[0].addr_type);
            gfx_clear(GFX_WHITE);
            if (ret == ESP_OK) {
                draw_centered("CONNECTED!", GFX_HEIGHT / 2 - 12, GFX_BLACK);
                gfx_text(devs[0].name, 40, GFX_HEIGHT / 2 + 12, GFX_GRAY(64));
            } else {
                draw_centered("FAILED", GFX_HEIGHT / 2 - 12, GFX_BLACK);
                draw_centered("Using BOOT key", GFX_HEIGHT / 2 + 12, GFX_GRAY(128));
            }
            gfx_flush();
            vTaskDelay(pdMS_TO_TICKS(2000));
            return;
        }

        // Redraw every 500ms
        if (wait_elapsed - last_flush_ms >= 500) {
            last_flush_ms = wait_elapsed;
            gfx_clear(GFX_WHITE);
            draw_centered("BLUETOOTH PAIRING", 16, GFX_BLACK);
            gfx_line(40, 32, GFX_WIDTH - 40, 32, GFX_GRAY(180));

            if (ndev == 0) {
                char buf[32];
                snprintf(buf, sizeof(buf), "Searching... %02ds", (15 - wait_elapsed / 1000));
                draw_centered(buf, GFX_HEIGHT / 2 - 12, GFX_GRAY(128));
                draw_centered("Press BOOT to skip", GFX_HEIGHT / 2 + 8, GFX_GRAY(64));
            } else {
                for (int i = 0; i < ndev && i < 4; i++) {
                    gfx_text(devs[i].name, 40, GFX_HEIGHT / 2 - 12 + i * 18, GFX_BLACK);
                    char rssi_buf[16];
                    snprintf(rssi_buf, sizeof(rssi_buf), "RSSI %d", devs[i].rssi);
                    gfx_text(rssi_buf, 280, GFX_HEIGHT / 2 - 12 + i * 18, GFX_GRAY(128));
                }
                draw_centered("Press BOOT to skip", GFX_HEIGHT / 2 + 60, GFX_GRAY(64));
            }
            gfx_flush();
        }
    }

    bt_hid_scan_stop();
    gfx_clear(GFX_WHITE);
    draw_centered("NO DEVICE FOUND", GFX_HEIGHT / 2 - 12, GFX_BLACK);
    draw_centered("Using BOOT key", GFX_HEIGHT / 2 + 12, GFX_GRAY(128));
    gfx_flush();
    vTaskDelay(pdMS_TO_TICKS(1500));
}

extern "C" void app_main(void)
{
    printf("\n");
    printf("====================================================\n");
    printf("  MonkeyMetal Console (M6: Launcher)\n");
    printf("  build " __DATE__ " " __TIME__ "\n");
    printf("====================================================\n");
    fflush(stdout);

    /* 1. NVS */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "[1/4] NVS OK");

    /* 2. Audio engine (before LCD so splash can play startup tones) */
    esp_err_t audio_ret = audio_engine_init();
    if (audio_ret != ESP_OK) {
        ESP_LOGW(TAG, "[2/9] Audio init failed (%s), continuing without audio",
                 esp_err_to_name(audio_ret));
    } else {
        ESP_LOGI(TAG, "[2/9] Audio engine OK");
    }

    /* 3. LCD + boot splash (with startup tones now available) */
    g_lcd.RLCD_Init();
    monkey_metal_play(&g_lcd);
    ESP_LOGI(TAG, "[3/9] LCD + splash OK");

    /* 4. SD */
    g_sd = new CustomSDPort(GBEMU_SD_MOUNTPOINT,
                            GBEMU_SD_CLK_GPIO, GBEMU_SD_CMD_GPIO,
                            GBEMU_SD_D0_GPIO, 1);
    ESP_LOGI(TAG, "[4/9] SD attempted at %s", GBEMU_SD_MOUNTPOINT);

    /* 5. Wi-Fi (async, GOT_IP arrives later) */
    gbemu_wifi_init();
    wifi_file_server_start();
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_set_sync_interval(3600000);  // 1 hour between syncs
    esp_sntp_init();
    // Trigger immediate sync (otherwise first poll can take minutes)
    sntp_set_sync_mode(SNTP_SYNC_MODE_IMMED);
    sntp_set_sync_interval(3600000);
    ESP_LOGI(TAG, "[5/9] WiFi init kicked off");

    /* 6. Init graphics engine */
    ESP_ERROR_CHECK(gfx_init());
    ESP_LOGI(TAG, "[6/9] Graphics engine OK");

    /* 7. Init BT HID host (non-fatal: Snake works fine with BOOT key) */
    esp_err_t bt_ret = bt_hid_init();
    if (bt_ret != ESP_OK) {
        ESP_LOGW(TAG, "[7/9] BT HID init failed (%s), continuing without BT",
                 esp_err_to_name(bt_ret));
    } else {
        ESP_LOGI(TAG, "[7/9] BT HID host OK");

        // Try reconnect bonded device first
        if (!bt_hid_reconnect_bonded()) {
            // No bonded device — scan and show pairing screen
            bt_hid_scan_start(30);
            ESP_LOGI(TAG, "BT: Scanning for gamepads...");

            // Show pairing screen with device list
            bt_pairing_screen();
        }
    }

    /* 8. Launcher + cart loop */
    const char *cart = "/sdcard/system/launcher";
    char cart_buf[128];
    ESP_LOGI(TAG, "[8/9] Entering launcher loop");

    while (cart) {
        // Copy to local buffer before init (init may clear the next-cart buffer)
        strncpy(cart_buf, cart, sizeof(cart_buf) - 1);
        cart_buf[sizeof(cart_buf) - 1] = '\0';

        ESP_ERROR_CHECK(mm_lua_init());
        ESP_LOGI(TAG, "Running cart: %s", cart_buf);
        esp_err_t err = mm_lua_run_cart(cart_buf);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "Cart %s error: %s", cart_buf, esp_err_to_name(err));
        }
        mm_lua_deinit();

        // Check if cart requested a switch
        const char *next = mm_lua_get_next_cart();
        if (next && next[0]) {
            cart = next;
        } else {
            cart = NULL;
        }
    }

    ESP_LOGI(TAG, "Launcher exited. System idle.");
    mm_lua_deinit(); // safety
    while (1) {
        bt_hid_poll_reconnect();
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
