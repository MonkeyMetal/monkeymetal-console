/// @file mm_lua_system.c
/// @brief Lua binding for system utilities (system.* table)
///
/// Lua API:
///   system.time_ms()     milliseconds since boot
///   system.exit()        signal cart to exit cleanly
///   system.log(msg)      print to serial (ESP_LOGI)

#include "lua.h"
#include "lauxlib.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "lua_sys";

static int l_system_time_ms(lua_State *L)
{
    int64_t ms = esp_timer_get_time() / 1000;
    lua_pushinteger(L, (lua_Integer)ms);
    return 1;
}

static int l_system_exit(lua_State *L)
{
    (void)L;
    // Set a global flag; the cart loop checks it after each frame
    lua_pushboolean(L, true);
    lua_setglobal(L, "__mm_exit_requested");
    return 0;
}

static int l_system_log(lua_State *L)
{
    const char *msg = luaL_checkstring(L, 1);
    ESP_LOGI(TAG, "[cart] %s", msg);
    return 0;
}

static const luaL_Reg system_funcs[] = {
    {"time_ms", l_system_time_ms},
    {"exit",    l_system_exit},
    {"log",     l_system_log},
    {NULL, NULL}
};

int mm_lua_open_system(lua_State *L)
{
    luaL_newlib(L, system_funcs);
    return 1;
}
