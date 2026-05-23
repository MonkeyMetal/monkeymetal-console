/*
 * MonkeyMetal Console - main.cpp (M1: Graphics Engine Demo)
 *
 * Boot sequence:
 *   1. NVS init
 *   2. ST7305 LCD bring-up + MonkeyMetal boot splash
 *   3. SD card mount at /sdcard
 *   4. Wi-Fi STA (async)
 *   5. Graphics engine demo (M1 validation)
 *
 * M1 demo shows:
 *   - 16bpp framebuffer → Bayer 4×4 dither → 1bpp LCD
 *   - Gradient fill (test dither quality)
 *   - Rectangles, circles, lines
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
#include "monkey_metal.h"
#include "gfx_engine.h"

static const char *TAG = "mm";

/* Global LCD instance - other subsystems reference via extern. */
DisplayPort g_lcd(GBEMU_LCD_MOSI_GPIO, GBEMU_LCD_SCK_GPIO,
                  GBEMU_LCD_DC_GPIO, GBEMU_LCD_CS_GPIO, GBEMU_LCD_RST_GPIO,
                  GBEMU_LCD_W, GBEMU_LCD_H);

static CustomSDPort *g_sd = nullptr;

static void m1_graphics_demo(void)
{
    ESP_LOGI(TAG, "=== M1 Graphics Engine Demo ===");

    // Initialize graphics engine
    ESP_ERROR_CHECK(gfx_init());

    // Test 1: Gradient (validates Bayer dither)
    ESP_LOGI(TAG, "Drawing gradient...");
    for (int y = 0; y < GFX_HEIGHT; y++) {
        uint16_t gray = (y * 65535) / GFX_HEIGHT;
        for (int x = 0; x < GFX_WIDTH; x++) {
            gfx_pixel(x, y, gray);
        }
    }
    gfx_flush();
    vTaskDelay(pdMS_TO_TICKS(2000));

    // Test 2: Geometric shapes
    ESP_LOGI(TAG, "Drawing shapes...");
    gfx_clear(GFX_WHITE);

    // Black rectangles
    gfx_rect(50, 50, 100, 80, GFX_BLACK, true);
    gfx_rect(200, 50, 100, 80, GFX_BLACK, false);

    // Gray circles (test dither on mid-tones)
    gfx_circle(100, 200, 40, GFX_GRAY(64), true);
    gfx_circle(250, 200, 40, GFX_GRAY(128), true);
    gfx_circle(350, 200, 40, GFX_GRAY(192), true);

    // Lines
    gfx_line(0, 0, GFX_WIDTH - 1, GFX_HEIGHT - 1, GFX_BLACK);
    gfx_line(GFX_WIDTH - 1, 0, 0, GFX_HEIGHT - 1, GFX_BLACK);

    gfx_flush();
    ESP_LOGI(TAG, "M1 demo complete. Screen should show gradient → shapes.");

    // Hold forever
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

extern "C" void app_main(void)
{
    printf("\n");
    printf("====================================================\n");
    printf("  MonkeyMetal Console (M1: Graphics Engine)\n");
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

    /* 2. LCD + boot splash */
    g_lcd.RLCD_Init();
    monkey_metal_play(&g_lcd);
    ESP_LOGI(TAG, "[2/4] LCD + splash OK");

    /* 3. SD */
    g_sd = new CustomSDPort(GBEMU_SD_MOUNTPOINT,
                            GBEMU_SD_CLK_GPIO, GBEMU_SD_CMD_GPIO,
                            GBEMU_SD_D0_GPIO, 1);
    ESP_LOGI(TAG, "[3/4] SD attempted at %s", GBEMU_SD_MOUNTPOINT);

    /* 4. Wi-Fi (async, GOT_IP arrives later) */
    gbemu_wifi_init();
    ESP_LOGI(TAG, "[4/4] WiFi init kicked off");

    /* M1 graphics demo */
    m1_graphics_demo();

    ESP_LOGE(TAG, "m1_graphics_demo returned unexpectedly");
}
