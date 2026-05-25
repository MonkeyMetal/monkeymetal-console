/// @file mm_sensors.h
/// @brief SHTC3 temperature/humidity + PCF85063 RTC sensor driver
#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/// @brief Initialize sensors (SHTC3 + RTC) on I2C port 0.
/// Safe to call multiple times.
void mm_sensors_init(void);

/// @brief Read temperature in Celsius. Returns 0.0 on failure.
float mm_sensors_read_temp_c(void);

/// @brief Read relative humidity in %. Returns 0.0 on failure.
float mm_sensors_read_humidity_pct(void);

/// @brief Read RTC time string. Returns "HH:MM:SS" or "--:--:--".
const char *mm_sensors_read_time_str(void);

/// @brief Check if sensors are ready.
bool mm_sensors_ready(void);

#ifdef __cplusplus
}
#endif
