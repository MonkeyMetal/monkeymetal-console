/*
 * wifi_bsp.c - WiFi STA 简易启动器, 失败自动重连, 拿到 IP 打 log.
 */
#include "wifi_bsp.h"
#include "user_secrets.h"   /* 由 main 提供 -- 注意 PRIV_REQUIRES 要包含 main 才能搜到 */

#include <string.h>
#include <stdbool.h>
#include <esp_log.h>
#include <esp_event.h>
#include <esp_netif.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static const char *TAG = "wifi_bsp";

static volatile bool s_connected = false;
static int           s_retry_cnt = 0;
#define MAX_RETRY 10

static void on_wifi_event(void *arg, esp_event_base_t base,
                          int32_t id, void *data)
{
    if (id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "STA started, connecting to \"%s\"...", GBEMU_WIFI_SSID);
        esp_wifi_connect();
    } else if (id == WIFI_EVENT_STA_DISCONNECTED) {
        s_connected = false;
        if (s_retry_cnt++ < MAX_RETRY) {
            ESP_LOGW(TAG, "disconnected, retry %d/%d", s_retry_cnt, MAX_RETRY);
            esp_wifi_connect();
        } else {
            ESP_LOGE(TAG, "max retry reached, giving up (will be re-enabled on next boot)");
        }
    }
}

static void on_ip_event(void *arg, esp_event_base_t base,
                        int32_t id, void *data)
{
    if (id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *evt = (ip_event_got_ip_t *)data;
        ESP_LOGI(TAG, "GOT IP: " IPSTR, IP2STR(&evt->ip_info.ip));
        s_retry_cnt = 0;
        s_connected = true;
    }
}

void gbemu_wifi_init(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &on_wifi_event, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &on_ip_event, NULL, NULL));

    wifi_config_t wc = {};
    strncpy((char *)wc.sta.ssid,     GBEMU_WIFI_SSID,     sizeof(wc.sta.ssid)     - 1);
    strncpy((char *)wc.sta.password, GBEMU_WIFI_PASSWORD, sizeof(wc.sta.password) - 1);
    wc.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wc));
    ESP_ERROR_CHECK(esp_wifi_start());
}

bool gbemu_wifi_is_connected(void)
{
    return s_connected;
}
