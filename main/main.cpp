/*
 * gbemu / main.cpp
 *
 * 启动顺序:
 *   1. NVS init (WiFi 需要)
 *   2. ST7305 LCD 上电 + 显黑白条验证
 *   3. SD 卡挂载到 /sdcard
 *   4. WiFi STA 连接 (异步)
 *   5. UDP 输入服务监听 8888 (异步, 待实现)
 *   6. gnuboy 主循环 (待实现, 当前先 stub)
 *
 * 第一阶段目标: 编译过 + 三大子系统全部上线 (LCD/SD/WiFi 各自串口打印 OK).
 * 第二阶段才接入 gnuboy 实际跑.
 */

#include <stdio.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_heap_caps.h>
#include <esp_system.h>
#include <nvs_flash.h>

#include "user_config.h"
#include "display_bsp.h"
#include "sdcard_bsp.h"
#include "wifi_bsp.h"
#include "gnuboy_sys_esp32idf.h"

static const char *TAG = "gbemu";

/* 主屏对象 - 横屏 400x300 */
DisplayPort g_lcd(GBEMU_LCD_MOSI_GPIO, GBEMU_LCD_SCK_GPIO,
                  GBEMU_LCD_DC_GPIO, GBEMU_LCD_CS_GPIO, GBEMU_LCD_RST_GPIO,
                  GBEMU_LCD_W, GBEMU_LCD_H);

/* SD 卡对象 - 1 bit 模式 */
static CustomSDPort *g_sd = nullptr;

/* ----------------------------------------------------------
 * 上电自检画面: 屏幕一半黑一半白, 看到说明 LCD + SPI + LUT 全通.
 * ---------------------------------------------------------- */
static void lcd_boot_pattern(void)
{
    g_lcd.RLCD_ColorClear(ColorWhite);
    for (int y = 0; y < GBEMU_LCD_H; y++) {
        for (int x = 0; x < GBEMU_LCD_W / 2; x++) {
            g_lcd.RLCD_SetPixel(x, y, ColorBlack);
        }
    }
    g_lcd.RLCD_Display();
}

/* ----------------------------------------------------------
 * 心跳任务 - 串口确认调度器 + 内存状态.
 * ---------------------------------------------------------- */
static void heartbeat_task(void *)
{
    uint32_t tick = 0;
    while (true) {
        ESP_LOGI(TAG, "[heartbeat] tick=%lu free_heap=%u psram=%u",
                 (unsigned long)tick++,
                 (unsigned)esp_get_free_heap_size(),
                 (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

extern "C" void app_main(void)
{
    /* ===== 0. 启动横幅 ===== */
    printf("\n");
    printf("====================================================\n");
    printf("  gbemu - GameBoy emulator on ESP32-S3-RLCD-4.2\n");
    printf("  build " __DATE__ " " __TIME__ "\n");
    printf("====================================================\n");
    fflush(stdout);

    /* ===== 1. NVS (WiFi 需要) ===== */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "[1/5] NVS OK");

    /* ===== 2. LCD ===== */
    g_lcd.RLCD_Init();
    lcd_boot_pattern();
    ESP_LOGI(TAG, "[2/5] LCD OK (boot pattern shown)");

    /* ===== 3. SD 卡 ===== */
    g_sd = new CustomSDPort(GBEMU_SD_MOUNTPOINT,
                            GBEMU_SD_CLK_GPIO, GBEMU_SD_CMD_GPIO,
                            GBEMU_SD_D0_GPIO, 1);
    ESP_LOGI(TAG, "[3/5] SD mount attempted at %s", GBEMU_SD_MOUNTPOINT);

    /* ===== 4. WiFi ===== */
    gbemu_wifi_init();
    ESP_LOGI(TAG, "[4/5] WiFi init kicked off (async)");

    /* ===== 5. gnuboy 平台层 (当前 stub) ===== */
    gnuboy_sys_init();
    ESP_LOGI(TAG, "[5/5] gnuboy sys-port stub initialized");

    /* heartbeat */
    xTaskCreatePinnedToCore(heartbeat_task, "heartbeat",
                            4096, nullptr, 1, nullptr, 0);

    ESP_LOGI(TAG, "app_main returning, FreeRTOS tasks now own the show.");
}
