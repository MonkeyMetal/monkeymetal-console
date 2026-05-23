/*
 * MonkeyMetal Console - main.cpp (M0 transitional skeleton)
 *
 * Boot sequence (current scaffold):
 *   1. NVS init
 *   2. ST7305 LCD bring-up + MonkeyMetal boot splash
 *   3. SD card mount at /sdcard
 *   4. Wi-Fi STA (async)
 *   5. Hand off to key_test_run() until the game engine is wired in
 *
 * Subsystems still to come (see docs/MonkeyMetal-Console-Design.md):
 *   - engine_gfx (M1): 16bpp framebuffer + Bayer 4x4 dither + async push
 *   - lua_runtime (M2): Lua 5.4 + gfx/input/system bindings
 *   - hal_bt_hid (M5): BLE HID host for controller pairing
 *   - hal_es8311 (M4): I2S audio
 *   - launcher / store (M6 / M7)
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
#include "key_test.h"

static const char *TAG = "mm";

/* Global LCD instance - other subsystems reference via extern. */
DisplayPort g_lcd(GBEMU_LCD_MOSI_GPIO, GBEMU_LCD_SCK_GPIO,
                  GBEMU_LCD_DC_GPIO, GBEMU_LCD_CS_GPIO, GBEMU_LCD_RST_GPIO,
                  GBEMU_LCD_W, GBEMU_LCD_H);

static CustomSDPort *g_sd = nullptr;

extern "C" void app_main(void)
{
    printf("\n");
    printf("====================================================\n");
    printf("  MonkeyMetal Console (M0 scaffold)\n");
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

    /* M0 placeholder: diagnostic loop until the game engine lands. */
    ESP_LOGI(TAG, "Entering KEY TEST mode (engine not yet wired in)");
    key_test_run(&g_lcd);

    ESP_LOGE(TAG, "key_test_run returned unexpectedly");
}
