/// @file mm_lua_system.c
/// @brief Lua binding for system utilities (system.* table)
///
/// Lua API:
///   system.time_ms()             milliseconds since boot
///   system.exit()                signal cart to exit cleanly
///   system.log(msg)              print to serial (ESP_LOGI)
///   system.run_cart(path)        switch to another cart
///   system.list_dir(path)        list directory entries (/sdcard/ only)
///   system.read_file(path)       read file as string (/sdcard/ only, ≤128KB)
///   system.write_file(path,data) write string to file (/sdcard/ only)
///   system.wifi_ip()             WiFi IP address string (or "0.0.0.0")
///   system.wifi_connected()      true if WiFi has IP
///   system.temp_c()              SHTC3 temperature in Celsius (float)
///   system.humidity_pct()        SHTC3 relative humidity % (float)
///   system.time_str()            HH:MM:SS from RTC (PCF85063)

#include "lua.h"
#include "lauxlib.h"
#include "mm_lua_runtime.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "wifi_bsp.h"
#include "mm_sensors.h"
#include <time.h>
#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static const char *TAG = "lua_sys";

#define MAX_FILE_SIZE  131072  // 128KB

// ── Path safety check ───────────────────────────────────────────────────────
static bool path_ok(const char *path)
{
    return (strncmp(path, "/sdcard/", 8) == 0 && strlen(path) > 8);
}

// ── system.time_ms ──────────────────────────────────────────────────────────
static int l_system_time_ms(lua_State *L)
{
    int64_t ms = esp_timer_get_time() / 1000;
    lua_pushinteger(L, (lua_Integer)ms);
    return 1;
}

// ── system.exit ─────────────────────────────────────────────────────────────
static int l_system_exit(lua_State *L)
{
    (void)L;
    lua_pushboolean(L, true);
    lua_setglobal(L, "__mm_exit_requested");
    return 0;
}

// ── system.log ──────────────────────────────────────────────────────────────
static int l_system_log(lua_State *L)
{
    const char *msg = luaL_checkstring(L, 1);
    ESP_LOGI(TAG, "[cart] %s", msg);
    return 0;
}

// ── system.run_cart ─────────────────────────────────────────────────────────
static int l_system_run_cart(lua_State *L)
{
    const char *path = luaL_checkstring(L, 1);
    if (!path_ok(path)) {
        ESP_LOGW(TAG, "run_cart rejected path: %s", path);
        lua_pushboolean(L, false);
        return 1;
    }
    mm_lua_request_cart(path);
    return 0;
}

// ── system.list_dir ─────────────────────────────────────────────────────────
static int l_system_list_dir(lua_State *L)
{
    const char *path = luaL_checkstring(L, 1);
    if (!path_ok(path)) {
        lua_newtable(L);
        return 1; // empty table
    }

    DIR *d = opendir(path);
    if (!d) {
        lua_newtable(L);
        return 1;
    }

    lua_newtable(L);
    int idx = 1;

    struct dirent *entry;
    while ((entry = readdir(d)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        lua_pushinteger(L, idx++);
        lua_newtable(L);

        lua_pushstring(L, entry->d_name);
        lua_setfield(L, -2, "name");

        lua_pushboolean(L, entry->d_type == DT_DIR);
        lua_setfield(L, -2, "is_dir");

        lua_settable(L, -3);
    }

    closedir(d);
    return 1;
}

// ── system.read_file ────────────────────────────────────────────────────────
static int l_system_read_file(lua_State *L)
{
    const char *path = luaL_checkstring(L, 1);
    if (!path_ok(path)) {
        lua_pushnil(L);
        lua_pushstring(L, "path rejected");
        return 2;
    }

    FILE *f = fopen(path, "rb");
    if (!f) {
        lua_pushnil(L);
        lua_pushstring(L, "open failed");
        return 2;
    }

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (fsize <= 0 || fsize > MAX_FILE_SIZE) {
        fclose(f);
        lua_pushnil(L);
        lua_pushstring(L, "size out of range");
        return 2;
    }

    char *buf = (char *)malloc(fsize + 1);
    if (!buf) {
        fclose(f);
        lua_pushnil(L);
        lua_pushstring(L, "alloc failed");
        return 2;
    }

    size_t n = fread(buf, 1, fsize, f);
    fclose(f);
    buf[n] = '\0';

    lua_pushstring(L, buf);
    free(buf);
    return 1;
}

// ── system.write_file ───────────────────────────────────────────────────────
static int l_system_write_file(lua_State *L)
{
    const char *path = luaL_checkstring(L, 1);
    const char *data = luaL_checkstring(L, 2);
    if (!path_ok(path)) {
        lua_pushboolean(L, false);
        return 1;
    }
    FILE *f = fopen(path, "wb");
    if (!f) {
        lua_pushboolean(L, false);
        return 1;
    }
    size_t len = strlen(data);
    size_t written = fwrite(data, 1, len, f);
    fclose(f);
    lua_pushboolean(L, written == len);
    return 1;
}

// ── Sensors (SHTC3 + RTC, I2C port 0) ────────────────────────────────────────
static void sensors_init_once(void)
{
    mm_sensors_init();
}

// ── system.temp_c ────────────────────────────────────────────────────────────
static int l_system_temp_c(lua_State *L)
{
    sensors_init_once();
    float t = mm_sensors_read_temp_c();
    lua_pushnumber(L, (lua_Number)t);
    return 1;
}

// ── system.humidity_pct ──────────────────────────────────────────────────────
static int l_system_humidity_pct(lua_State *L)
{
    sensors_init_once();
    float h = mm_sensors_read_humidity_pct();
    lua_pushnumber(L, (lua_Number)h);
    return 1;
}

// ── system.time_str (RTC time) ───────────────────────────────────────────────
static int l_system_time_str(lua_State *L)
{
    sensors_init_once();
    const char *t = mm_sensors_read_time_str();
    lua_pushstring(L, t);
    return 1;
}

// ── system.wifi_ip ──────────────────────────────────────────────────────────
static int l_system_wifi_ip(lua_State *L)
{
    lua_pushstring(L, gbemu_wifi_get_ip());
    return 1;
}

// ── system.wifi_connected ───────────────────────────────────────────────────
static int l_system_wifi_connected(lua_State *L)
{
    lua_pushboolean(L, gbemu_wifi_is_connected());
    return 1;
}

// ── Register table ──────────────────────────────────────────────────────────
static const luaL_Reg system_funcs[] = {
    {"time_ms",   l_system_time_ms},
    {"exit",      l_system_exit},
    {"log",       l_system_log},
    {"run_cart",  l_system_run_cart},
    {"list_dir",  l_system_list_dir},
    {"read_file",  l_system_read_file},
    {"write_file",     l_system_write_file},
    {"wifi_ip",        l_system_wifi_ip},
    {"wifi_connected", l_system_wifi_connected},
    {"temp_c",         l_system_temp_c},
    {"humidity_pct",   l_system_humidity_pct},
    {"time_str",       l_system_time_str},
    {NULL, NULL}
};

int mm_lua_open_system(lua_State *L)
{
    luaL_newlib(L, system_funcs);
    return 1;
}
