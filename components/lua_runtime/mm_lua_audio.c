/// @file mm_lua_audio.c
/// @brief Lua binding for audio_engine (audio.* table)
///
/// Lua API:
///   audio.tone(freq, duration_ms, volume)  -- play pure tone
///   audio.stop()                           -- stop current tone
///   audio.master(volume)                   -- set master volume 0-100

#include "lua.h"
#include "lauxlib.h"
#include "audio_engine.h"

static int l_audio_tone(lua_State *L)
{
    int freq = (int)luaL_checkinteger(L, 1);
    int dur  = (int)luaL_optinteger(L, 2, 200);
    int vol  = (int)luaL_optinteger(L, 3, 80);
    audio_tone_play(freq, dur, vol);
    return 0;
}

static int l_audio_stop(lua_State *L)
{
    (void)L;
    audio_tone_stop();
    return 0;
}

static int l_audio_master(lua_State *L)
{
    int vol = (int)luaL_checkinteger(L, 1);
    audio_engine_set_volume(vol);
    return 0;
}

static const luaL_Reg audio_funcs[] = {
    {"tone",   l_audio_tone},
    {"stop",   l_audio_stop},
    {"master", l_audio_master},
    {NULL, NULL}
};

int mm_lua_open_audio(lua_State *L)
{
    luaL_newlib(L, audio_funcs);
    return 1;
}
