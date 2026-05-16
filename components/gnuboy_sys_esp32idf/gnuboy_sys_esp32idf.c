/*
 * gnuboy_sys_esp32idf.c - 平台移植层占位实现.
 *
 * 当前阶段(骨架): 只提供 init/run stub, 让骨架编译通过.
 * 下一阶段(任务 #9): 实际接入 gnuboy 核心:
 *   - vid_init/begin/end : 把 gnuboy fb 指向 16bpp 中间缓冲
 *   - dither task        : Bayer 4x4 -> 1bpp -> ST7305
 *   - ev_poll            : 读 UDP 输入队列, 派发到 ev_postevent
 *   - pcm_*              : ES8311 + I2S 音频(后期)
 */
#include "gnuboy_sys_esp32idf.h"
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static const char *TAG = "gnuboy_sys";

void gnuboy_sys_init(void)
{
    ESP_LOGI(TAG, "init stub - dither/input/audio not wired yet (skeleton phase)");
}

void gnuboy_sys_run(const char *rom_path)
{
    ESP_LOGW(TAG, "run stub: rom_path=%s, gnuboy core not linked yet", rom_path);
    /* 真实实现将走 gnuboy 的 emu_init -> loader_init(rom_path) -> emu_run() */
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
