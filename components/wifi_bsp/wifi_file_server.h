/// @file wifi_file_server.h
/// @brief Simple TCP file receiver for OTA game updates over WiFi.
/// Listens on port 9999, accepts file uploads to /sdcard/.
#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/// @brief Start the TCP file server task (non-blocking).
/// Requires WiFi to be connected (GOT_IP) before calling.
esp_err_t wifi_file_server_start(void);

/// @brief Stop the file server.
void wifi_file_server_stop(void);

#ifdef __cplusplus
}
#endif
