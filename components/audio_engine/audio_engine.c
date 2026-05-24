/// @file audio_engine.c
/// @brief MonkeyMetal audio engine — ES8311 via esp_codec_dev + codec_board
///
/// Pins (S3_RLCD_4_2):
///   I2C SDA=13, SCL=14
///   I2S MCLK=16, BCLK=9, WS=45, DOUT=8, DIN=10
///   PA enable=46

#include "audio_engine.h"
#include "codec_board.h"
#include "codec_init.h"
#include "esp_codec_dev.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>

static const char *TAG = "audio";

#define AUDIO_SAMPLE_RATE   16000
#define AUDIO_CHANNELS      1
#define AUDIO_BITS          16
#define PA_GPIO             GPIO_NUM_46

static esp_codec_dev_handle_t s_play = NULL;
static bool s_inited = false;

// Tone generation task
static TaskHandle_t  s_tone_task = NULL;
static volatile bool s_tone_stop = false;

typedef struct {
    int freq_hz;
    int duration_ms;
    int volume;
} tone_params_t;

static i2c_master_bus_handle_t s_i2c_bus = NULL;

// ── PA control ───────────────────────────────────────────────────────────────
static void pa_enable(bool en)
{
    gpio_config_t cfg = {
        .pin_bit_mask = (1ULL << PA_GPIO),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLDOWN_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&cfg);
    gpio_set_level(PA_GPIO, en ? 1 : 0);
}

// ── Init ─────────────────────────────────────────────────────────────────────
esp_err_t audio_engine_init(void)
{
    if (s_inited) return ESP_OK;

    // Set board type so codec_board picks up correct pin config
    set_codec_board_type("S3_RLCD_4_2");

    // Init I2C (codec_board manages the bus internally for its own use,
    // but we also need to pass it a handle for the I2C master bus)
    int ret = init_i2c(0);
    if (ret != 0) {
        ESP_LOGE(TAG, "I2C init failed: %d", ret);
        return ESP_FAIL;
    }
    s_i2c_bus = (i2c_master_bus_handle_t)get_i2c_bus_handle(0);

    // Init codec (I2S + ES8311). Must use TDM mode — ES8311 on
    // Waveshare RLCD-4.2 is wired for TDM I2S with 4 slots.
    codec_init_cfg_t cfg = {};
    cfg.in_mode  = CODEC_I2S_MODE_TDM;
    cfg.out_mode = CODEC_I2S_MODE_TDM;
    cfg.in_use_tdm  = true;
    cfg.reuse_dev   = false;
    ret = init_codec(&cfg);
    if (ret != 0) {
        ESP_LOGE(TAG, "Codec init failed: %d", ret);
        return ESP_FAIL;
    }

    s_play = get_playback_handle();
    if (!s_play) {
        ESP_LOGE(TAG, "No playback handle");
        return ESP_FAIL;
    }

    // Open playback at 16 kHz mono 16-bit
    esp_codec_dev_sample_info_t info = {
        .bits_per_sample = AUDIO_BITS,
        .channel         = AUDIO_CHANNELS,
        .channel_mask    = 0,
        .sample_rate     = AUDIO_SAMPLE_RATE,
        .mclk_multiple   = 0,
    };
    ESP_ERROR_CHECK(esp_codec_dev_open(s_play, &info));
    ESP_ERROR_CHECK(esp_codec_dev_set_out_vol(s_play, 80));

    pa_enable(true);
    s_inited = true;
    ESP_LOGI(TAG, "Audio engine ready: %d Hz %dch %dbit, PA on GPIO%d",
             AUDIO_SAMPLE_RATE, AUDIO_CHANNELS, AUDIO_BITS, PA_GPIO);
    return ESP_OK;
}

void audio_engine_set_volume(int volume)
{
    if (!s_play) return;
    if (volume < 0)   volume = 0;
    if (volume > 100) volume = 100;
    esp_codec_dev_set_out_vol(s_play, volume);
}

// ── Tone generation ───────────────────────────────────────────────────────────
static void tone_task(void *arg)
{
    tone_params_t *p = (tone_params_t *)arg;
    const int freq     = p->freq_hz;
    const int dur_ms   = p->duration_ms;
    const int vol      = p->volume;
    free(p);

    const int sr        = AUDIO_SAMPLE_RATE;
    const int buf_samp  = sr / 20;              // 50 ms chunks
    int16_t  *buf       = (int16_t *)malloc(buf_samp * sizeof(int16_t));
    if (!buf) { vTaskDelete(NULL); return; }

    const float amp   = (vol / 100.0f) * 16000.0f;
    const float step  = 2.0f * 3.14159265f * freq / sr;
    float phase       = 0.0f;
    int   total_samp  = (dur_ms > 0) ? (sr * dur_ms / 1000) : INT32_MAX;
    int   written     = 0;

    esp_codec_dev_set_out_vol(s_play, vol);

    while (!s_tone_stop && written < total_samp) {
        int n = buf_samp;
        if (dur_ms > 0 && written + n > total_samp) n = total_samp - written;
        for (int i = 0; i < n; i++) {
            buf[i] = (int16_t)(sinf(phase) * amp);
            phase += step;
            if (phase > 2.0f * 3.14159265f) phase -= 2.0f * 3.14159265f;
        }
        esp_codec_dev_write(s_play, buf, n * sizeof(int16_t));
        written += n;
    }

    free(buf);
    s_tone_task = NULL;
    vTaskDelete(NULL);
}

void audio_tone_play(int freq_hz, int duration_ms, int volume)
{
    if (!s_inited) return;
    audio_tone_stop();

    tone_params_t *p = (tone_params_t *)malloc(sizeof(tone_params_t));
    if (!p) return;
    p->freq_hz     = freq_hz;
    p->duration_ms = duration_ms;
    p->volume      = volume;

    s_tone_stop = false;
    xTaskCreate(tone_task, "tone", 4096, p, 5, &s_tone_task);
}

void audio_tone_stop(void)
{
    if (s_tone_task) {
        s_tone_stop = true;
        // Wait for task to exit (max 200 ms)
        for (int i = 0; i < 20 && s_tone_task; i++) {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}

// ── Raw PCM write ─────────────────────────────────────────────────────────────
esp_err_t audio_pcm_write(const int16_t *pcm, size_t samples,
                          int sample_rate, int channels)
{
    if (!s_inited || !s_play) return ESP_ERR_INVALID_STATE;
    // Reconfigure if needed (simple: just write, codec already open at 16k/1ch)
    int bytes = (int)(samples * sizeof(int16_t));
    int ret = esp_codec_dev_write(s_play, (void *)pcm, bytes);
    return (ret == bytes) ? ESP_OK : ESP_FAIL;
}

// ── Deinit ────────────────────────────────────────────────────────────────────
void audio_engine_deinit(void)
{
    if (!s_inited) return;
    audio_tone_stop();
    pa_enable(false);
    if (s_play) esp_codec_dev_close(s_play);
    deinit_codec();
    deinit_i2c(0);
    s_play  = NULL;
    s_inited = false;
    ESP_LOGI(TAG, "Audio engine deinit");
}
