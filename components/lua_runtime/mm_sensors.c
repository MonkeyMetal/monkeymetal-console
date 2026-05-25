/// @file mm_sensors.c
/// @brief SHTC3 temp/humidity (I2C 0x70) + PCF85063 RTC (I2C 0x51)

#include "mm_sensors.h"
#include "wifi_bsp.h"
#include "esp_log.h"
#include "driver/i2c_master.h"
#include "esp_timer.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <time.h>
#include <string.h>
#include <stdio.h>

static const char *TAG = "sensors";

#define SHTC3_ADDR      0x70
#define PCF85063_ADDR   0x51
#define I2C_PORT        0

static i2c_master_dev_handle_t s_shtc3_dev = NULL;
static i2c_master_dev_handle_t s_rtc_dev   = NULL;
static bool   s_ready    = false;

// Cached sensor values (refreshed every ~1s)
static float  s_temp_c   = 0.0f;
static float  s_hum_pct  = 0.0f;
static int64_t s_last_read_ms = 0;
static char   s_time_buf[16] = "--:--:--";

// ── SHTC3 commands ────────────────────────────────────────────
#define SHTC3_WAKEUP    0x3517
#define SHTC3_SLEEP     0xB098
#define SHTC3_SOFTRESET 0x805D
#define SHTC3_READ_ID   0xEFC8
#define SHTC3_MEAS_TFIRST 0x7866

// ── CRC-8 ─────────────────────────────────────────────────────
static uint8_t crc8(const uint8_t *data, int len)
{
    uint8_t crc = 0xFF;
    for (int i = 0; i < len; i++) {
        crc ^= data[i];
        for (int b = 0; b < 8; b++)
            crc = (crc & 0x80) ? ((crc << 1) ^ 0x31) : (crc << 1);
    }
    return crc;
}

static uint8_t bcd2bin(uint8_t v) { return ((v >> 4) * 10) + (v & 0x0F); }

// ── Init ───────────────────────────────────────────────────────
void mm_sensors_init(void)
{
    if (s_ready) return;

    i2c_master_bus_handle_t bus;
    esp_err_t ret = i2c_master_get_bus_handle(I2C_PORT, &bus);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "I2C bus %d not ready: %s", I2C_PORT, esp_err_to_name(ret));
        return;
    }

    // ── SHTC3 ──────────────────────────────────────────────────
    i2c_device_config_t sc = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = SHTC3_ADDR,
        .scl_speed_hz    = 100000,
    };
    ret = i2c_master_bus_add_device(bus, &sc, &s_shtc3_dev);
    if (ret != ESP_OK) { ESP_LOGW(TAG, "SHTC3 add fail: %s", esp_err_to_name(ret)); }

    // ── RTC ────────────────────────────────────────────────────
    i2c_device_config_t rc = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = PCF85063_ADDR,
        .scl_speed_hz    = 100000,
    };
    ret = i2c_master_bus_add_device(bus, &rc, &s_rtc_dev);
    if (ret != ESP_OK) { ESP_LOGW(TAG, "RTC add fail: %s", esp_err_to_name(ret)); }

    // ── SHTC3 init sequence: wake → reset → verify ID ─────────
    if (s_shtc3_dev) {
        // Wakeup
        uint8_t wu[2] = {0x35, 0x17};
        i2c_master_transmit(s_shtc3_dev, wu, 2, 20);
        vTaskDelay(pdMS_TO_TICKS(1));

        // Soft reset
        uint8_t sr[2] = {0x80, 0x5D};
        i2c_master_transmit(s_shtc3_dev, sr, 2, 20);
        vTaskDelay(pdMS_TO_TICKS(1));

        // Read ID (should return 0x0807)
        uint8_t id_cmd[2] = {0xEF, 0xC8};
        i2c_master_transmit(s_shtc3_dev, id_cmd, 2, 20);
        uint8_t id[3] = {0};
        ret = i2c_master_receive(s_shtc3_dev, id, 3, 20);
        if (ret == ESP_OK) {
            uint16_t sid = ((uint16_t)id[0] << 8) | id[1];
            ESP_LOGI(TAG, "SHTC3 ID: %04X", sid);
        } else {
            ESP_LOGW(TAG, "SHTC3 ID read failed, continuing anyway");
        }

        // Sleep
        uint8_t sl[2] = {0xB0, 0x98};
        i2c_master_transmit(s_shtc3_dev, sl, 2, 20);

        s_ready = true;
        ESP_LOGI(TAG, "SHTC3 + RTC ready");
    }
}

// ── Combined read (temp + humidity in one transaction) ─────────
static void sensor_read(void)
{
    if (!s_shtc3_dev) return;

    int64_t now = esp_timer_get_time() / 1000;
    if (now - s_last_read_ms < 1000) return;  // cache 1s
    s_last_read_ms = now;

    // Wakeup
    uint8_t wu[2] = {0x35, 0x17};
    esp_err_t ret = i2c_master_transmit(s_shtc3_dev, wu, 2, 20);
    if (ret != ESP_OK) { ESP_LOGW(TAG, "SHTC3 wakeup err %d", ret); return; }
    vTaskDelay(pdMS_TO_TICKS(1));

    // Measure (temp first, polling)
    uint8_t mc[2] = {0x78, 0x66};
    ret = i2c_master_transmit(s_shtc3_dev, mc, 2, 20);
    if (ret != ESP_OK) { ESP_LOGW(TAG, "SHTC3 meas err %d", ret); return; }
    vTaskDelay(pdMS_TO_TICKS(15));  // max 12.1ms per datasheet

    // Read 6 bytes
    uint8_t buf[6] = {0};
    ret = i2c_master_receive(s_shtc3_dev, buf, 6, 50);
    if (ret != ESP_OK) { ESP_LOGW(TAG, "SHTC3 read err %d", ret); return; }

    // Sleep
    uint8_t sl[2] = {0xB0, 0x98};
    i2c_master_transmit(s_shtc3_dev, sl, 2, 20);

    // Parse temperature (bytes 0-2)
    // Formula from Sensirion datasheet: 175 * raw / 65536 - 45
    // Apply -4C offset for self-heating compensation (per Waveshare calibration)
    if (crc8(buf, 2) == buf[2]) {
        uint16_t tr = ((uint16_t)buf[0] << 8) | buf[1];
        s_temp_c = 175.0f * (float)tr / 65536.0f - 45.0f - 4.0f;
    }

    // Parse humidity (bytes 3-5)
    if (crc8(buf + 3, 2) == buf[5]) {
        uint16_t hr = ((uint16_t)buf[3] << 8) | buf[4];
        s_hum_pct = 100.0f * (float)hr / 65536.0f;
    }
}

// ── Public API ─────────────────────────────────────────────────
float mm_sensors_read_temp_c(void)      { sensor_read(); return s_temp_c; }
float mm_sensors_read_humidity_pct(void){ sensor_read(); return s_hum_pct; }

const char *mm_sensors_read_time_str(void)
{
    // Prefer SNTP time when WiFi is connected
    if (gbemu_wifi_is_connected()) {
        time_t now;
        struct tm ti;
        time(&now);
        localtime_r(&now, &ti);
        if (ti.tm_year >= 126) {  // year 2026+ = valid SNTP time
            snprintf(s_time_buf, sizeof(s_time_buf), "%02d:%02d:%02d",
                     ti.tm_hour, ti.tm_min, ti.tm_sec);
            return s_time_buf;
        }
    }

    // Fallback: RTC hardware clock
    if (!s_rtc_dev) return s_time_buf;
    uint8_t reg = 0x04;
    uint8_t rtc[3] = {0};
    esp_err_t ret = i2c_master_transmit_receive(s_rtc_dev, &reg, 1, rtc, 3, 50);
    if (ret != ESP_OK) return s_time_buf;

    snprintf(s_time_buf, sizeof(s_time_buf), "%02d:%02d:%02d",
             bcd2bin(rtc[2] & 0x3F), bcd2bin(rtc[1] & 0x7F), bcd2bin(rtc[0] & 0x7F));
    return s_time_buf;
}

bool mm_sensors_ready(void) { return s_ready; }
