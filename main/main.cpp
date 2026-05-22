/*
 * gbemu / main.cpp
 *
 * Boot sequence:
 *   1. NVS init (required by Wi-Fi)
 *   2. ST7305 LCD power up
 *   3. SD card mount at /sdcard (ROM source)
 *   4. Wi-Fi STA (async; task #15 UDP input depends on it)
 *   5. gnuboy sys-port init: PSRAM framebuffer, input queue
 *   6. gnuboy emu_init / vid_init / pcm_init
 *   7. gnuboy_sys_run("/sdcard/rom.gbc") -> emu_run, never returns
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
#include "monkey_metal.h"
#include "key_test.h"

/* gnuboy core hooks - declared in sys.h, implemented in gnuboy_sys_esp32idf.
 * Need to be called once before emu_run. */
extern "C" {
void vid_preinit(void);
void vid_init(void);
void pcm_init(void);
void emu_init(void);
}

static const char *TAG = "gbemu";

/* Global LCD instance - sys layer references via extern. */
DisplayPort g_lcd(GBEMU_LCD_MOSI_GPIO, GBEMU_LCD_SCK_GPIO,
                  GBEMU_LCD_DC_GPIO, GBEMU_LCD_CS_GPIO, GBEMU_LCD_RST_GPIO,
                  GBEMU_LCD_W, GBEMU_LCD_H);

/* SD card */
static CustomSDPort *g_sd = nullptr;

extern "C" void app_main(void)
{
    /* ---- Boot banner ---- */
    printf("\n");
    printf("====================================================\n");
    printf("  gbemu - GameBoy emulator on ESP32-S3-RLCD-4.2\n");
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
    ESP_LOGI(TAG, "[1/5] NVS OK");

    /* 2. LCD */
    g_lcd.RLCD_Init();
    ESP_LOGI(TAG, "[2/5] LCD OK");

    /* 2b. Boot splash - so the user knows the board is alive even before
     *     SD mount finishes (slow cards can stall for ~1 s). */
    monkey_metal_play(&g_lcd);

    /* 3. SD */
    g_sd = new CustomSDPort(GBEMU_SD_MOUNTPOINT,
                            GBEMU_SD_CLK_GPIO, GBEMU_SD_CMD_GPIO,
                            GBEMU_SD_D0_GPIO, 1);
    ESP_LOGI(TAG, "[3/5] SD attempted at %s (need TF card with rom.gbc on root)",
             GBEMU_SD_MOUNTPOINT);

    /* 4. Wi-Fi (async, will GOT_IP later while emu is running) */
    gbemu_wifi_init();
    ESP_LOGI(TAG, "[4/5] WiFi init kicked off");

    /* 5. gnuboy sys-port + core init */
    gnuboy_sys_init();
    vid_preinit();
    vid_init();
    pcm_init();
    emu_init();
    ESP_LOGI(TAG, "[5/5] gnuboy sys-port + vid/pcm/emu initialized");

    /* 6. enter emu_run (never returns) */
    ESP_LOGI(TAG, "Entering KEY TEST mode (gnuboy temporarily disabled)");
    /* gnuboy emu has a known crash on first cpu_emulate (rom.bank corrupt).
     * Run the input/diagnostic loop instead so we can test buttons + LCD. */
    key_test_run(&g_lcd);

    /* once key_test is satisfied, swap back to:
     *   gnuboy_sys_run(GBEMU_DEFAULT_ROM_PATH);
     */

    ESP_LOGE(TAG, "key_test_run returned unexpectedly");
}
