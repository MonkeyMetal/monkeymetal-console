/// @file port_bsp.cpp
/// @brief C interface wrapper for DisplayPort C++ class

#include "port_bsp.h"
#include "display_bsp.h"
#include <string.h>

// External: global LCD instance from main.cpp
extern DisplayPort g_lcd;

esp_err_t port_bsp_lcd_flush(const uint8_t *buffer, size_t len)
{
    if (!buffer || len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    g_lcd.lcd_flush_raw(buffer, (int)len);

    return ESP_OK;
}
