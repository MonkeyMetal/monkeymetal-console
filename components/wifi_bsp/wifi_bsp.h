/*
 * wifi_bsp.h - 简易 WiFi STA 启动接口.
 *
 * 当前实现: 从 main/user_secrets.h 读取 SSID/Password, 启动 STA, 自动重连.
 * 后续: 可扩展成 NVS 配置 / SmartConfig / WebConfig.
 */
#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 启动 WiFi STA, 异步连接. 不阻塞. */
void gbemu_wifi_init(void);

/* 当前是否已经拿到 IP. */
bool gbemu_wifi_is_connected(void);

/* 获取当前 IP 地址字符串 (e.g. "192.168.1.100"). 未连接返回 "0.0.0.0". */
const char *gbemu_wifi_get_ip(void);

#ifdef __cplusplus
}
#endif
