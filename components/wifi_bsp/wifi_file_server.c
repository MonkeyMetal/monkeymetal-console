/// @file wifi_file_server.c
/// @brief TCP file receiver: listens on port 9999, receives files, writes to /sdcard/.
///
/// Protocol (plain TCP, one file per connection):
///   Client → Server:  "<path>\n<file-content>"
///   Server → Client:  "OK\n" or "ERR:<reason>\n"
///   Connection closes.
///
/// Client example (Python):
///   sock = socket.create_connection((ip, 9999))
///   sock.sendall(b"/games/iron_heart/src/render.lua\n" + file_bytes)
///   resp = sock.recv(1024)
///   sock.close()

#include "wifi_file_server.h"
#include "esp_log.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static const char *TAG = "file_srv";
#define PORT         9999
#define BUF_SIZE     4096
#define MAX_PATH     256
#define MAX_FILE     (128 * 1024)  // 128 KB max per file

static volatile bool s_running = false;

static void file_server_task(void *arg)
{
    (void)arg;

    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        ESP_LOGE(TAG, "socket() failed");
        vTaskDelete(NULL);
        return;
    }

    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        ESP_LOGE(TAG, "bind() failed");
        close(listen_fd);
        vTaskDelete(NULL);
        return;
    }

    if (listen(listen_fd, 2) < 0) {
        ESP_LOGE(TAG, "listen() failed");
        close(listen_fd);
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "File server listening on port %d", PORT);
    s_running = true;

    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    while (s_running) {
        int client_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0) {
            if (s_running) ESP_LOGW(TAG, "accept() failed");
            continue;
        }

        ESP_LOGI(TAG, "Client connected from %s", inet_ntoa(client_addr.sin_addr));

        // ── Read first line: path ──────────────────────────────────
        char path_buf[MAX_PATH] = {0};
        int  path_len = 0;

        while (path_len < MAX_PATH - 1) {
            char ch;
            int n = recv(client_fd, &ch, 1, 0);
            if (n <= 0) break;
            if (ch == '\n') break;
            path_buf[path_len++] = ch;
        }
        path_buf[path_len] = '\0';

        // ── Valid path check ───────────────────────────────────────
        char full_path[MAX_PATH + 16];
        if (strncmp(path_buf, "/", 1) == 0) {
            // Absolute path relative to /sdcard
            snprintf(full_path, sizeof(full_path), "/sdcard%s", path_buf);
        } else if (path_buf[0] != '\0') {
            snprintf(full_path, sizeof(full_path), "/sdcard/%s", path_buf);
        } else {
            send(client_fd, "ERR:empty path\n", 15, 0);
            close(client_fd);
            continue;
        }

        ESP_LOGI(TAG, "Receiving: %s → %s", path_buf, full_path);

        // ── Read file content ──────────────────────────────────────
        char *file_buf = (char *)malloc(MAX_FILE);
        if (!file_buf) {
            send(client_fd, "ERR:malloc\n", 11, 0);
            close(client_fd);
            continue;
        }

        int total = 0;
        int remaining = MAX_FILE;
        while (remaining > 0) {
            int n = recv(client_fd, file_buf + total, remaining, 0);
            if (n <= 0) break;
            total += n;
            remaining -= n;
            if (remaining <= 0) {
                ESP_LOGW(TAG, "File exceeds max size %d, truncating", MAX_FILE);
                break;
            }
        }

        // ── Write to SD card ───────────────────────────────────────
        if (total > 0) {
            // Ensure parent directories exist (simple mkdir)
            char dir_path[MAX_PATH + 16];
            strncpy(dir_path, full_path, sizeof(dir_path));
            dir_path[sizeof(dir_path) - 1] = '\0';
            char *last_slash = strrchr(dir_path, '/');
            if (last_slash) {
                *last_slash = '\0';
                // mkdir -p style: try creating each level
                char *p = dir_path;
                while (*p) {
                    if (*p == '/' && p > dir_path) {
                        *p = '\0';
                        mkdir(dir_path, 0755);
                        *p = '/';
                    }
                    p++;
                }
                mkdir(dir_path, 0755);
            }

            FILE *f = fopen(full_path, "wb");
            if (!f) {
                ESP_LOGE(TAG, "fopen(%s) failed", full_path);
                send(client_fd, "ERR:open\n", 10, 0);
            } else {
                size_t written = fwrite(file_buf, 1, total, f);
                fclose(f);
                if (written == (size_t)total) {
                    send(client_fd, "OK\n", 3, 0);
                    ESP_LOGI(TAG, "Saved %d bytes to %s", total, full_path);
                } else {
                    send(client_fd, "ERR:write\n", 11, 0);
                    ESP_LOGE(TAG, "Write mismatch: %d/%d bytes", (int)written, total);
                }
            }
        } else {
            send(client_fd, "ERR:empty\n", 11, 0);
        }

        free(file_buf);
        close(client_fd);
    }

    close(listen_fd);
    vTaskDelete(NULL);
}

esp_err_t wifi_file_server_start(void)
{
    if (xTaskCreate(file_server_task, "file_srv", 4096, NULL, 5, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create file server task");
        return ESP_FAIL;
    }
    return ESP_OK;
}

void wifi_file_server_stop(void)
{
    s_running = false;
}
