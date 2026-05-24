/// @file audio_engine.h
/// @brief MonkeyMetal audio engine public API — M4
///
/// Hardware: ES8311 codec via I2C (SDA=13, SCL=14) + I2S (MCLK=16, BCLK=9, WS=45, DOUT=8)
/// PA enable: GPIO 46

#pragma once

#include "esp_err.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/// @brief Initialize audio engine: I2C, I2S, ES8311 codec, PA.
/// @return ESP_OK on success
esp_err_t audio_engine_init(void);

/// @brief Set master volume (0–100).
void audio_engine_set_volume(int volume);

/// @brief Play a pure tone (square wave, non-blocking).
/// @param freq_hz  Frequency in Hz (e.g. 440 for A4)
/// @param duration_ms  Duration; 0 = play until audio_tone_stop()
/// @param volume  0–100
void audio_tone_play(int freq_hz, int duration_ms, int volume);

/// @brief Stop tone immediately.
void audio_tone_stop(void);

/// @brief Write raw PCM samples directly to I2S (blocking).
/// @param pcm     Pointer to 16-bit signed PCM samples
/// @param samples Number of samples (not bytes)
/// @param sample_rate  Sample rate in Hz
/// @param channels     1=mono, 2=stereo
esp_err_t audio_pcm_write(const int16_t *pcm, size_t samples,
                          int sample_rate, int channels);

/// @brief Deinitialize audio engine.
void audio_engine_deinit(void);

#ifdef __cplusplus
}
#endif
