/*
 * MonkeyMetal Console - main.cpp (M2: Lua Runtime)
 *
 * Boot sequence:
 *   1. NVS init
 *   2. ST7305 LCD bring-up + MonkeyMetal boot splash
 *   3. SD card mount at /sdcard
 *   4. Wi-Fi STA (async)
 *   5. Init graphics engine
 *   6. Init Lua VM
 *   7. Run cart from /sdcard/games/hello/cart.lua  (M2 validation)
 *      (M6 launcher will replace the hardcoded path)
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
#include "mm_lua_runtime.h"

static const char *TAG = "mm";

/* Global LCD instance - referenced by port_bsp and other subsystems via extern */
DisplayPort g_lcd(GBEMU_LCD_MOSI_GPIO, GBEMU_LCD_SCK_GPIO,
                  GBEMU_LCD_DC_GPIO, GBEMU_LCD_CS_GPIO, GBEMU_LCD_RST_GPIO,
                  GBEMU_LCD_W, GBEMU_LCD_H);

static CustomSDPort *g_sd = nullptr;

extern "C" void app_main(void)
{
    printf("\n");
    printf("====================================================\n");
    printf("  MonkeyMetal Console (M3: Snake)\n");
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

    /* 5. Init graphics engine */
    ESP_ERROR_CHECK(gfx_init());
    ESP_LOGI(TAG, "[5/6] Graphics engine OK");

    /* 6. Init Lua VM */
    ESP_ERROR_CHECK(mm_lua_init());
    ESP_LOGI(TAG, "[6/6] Lua VM OK");

    /* 7. Run Snake cart (M3)
     *    SD card must have /sdcard/games/snake/cart.lua */
    const char *cart = "/sdcard/games/snake";
    ESP_LOGI(TAG, "Running cart: %s", cart);
    esp_err_t err = mm_lua_run_cart(cart);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Cart failed, halting");
    }

    mm_lua_deinit();
    ESP_LOGI(TAG, "Cart exited. System idle.");
    while (1) { vTaskDelay(pdMS_TO_TICKS(1000)); }
}
