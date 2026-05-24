/// @file mm_lua_gfx.c
/// @brief Lua binding for gfx_engine (gfx.* table)
///
/// Lua API:
///   gfx.clear(c)              c: 0–255 gray (0=black, 255=white)
///   gfx.pixel(x, y, c)
///   gfx.line(x1,y1, x2,y2, c)
///   gfx.rect(x, y, w, h, c, fill)   fill: bool, default false
///   gfx.circle(cx, cy, r, c, fill)  fill: bool, default false
///   gfx.text(s, x, y, c)      8x8 font string (ASCII only)
///   gfx.flip()                push frame to LCD (~30 ms)

#include "lua.h"
#include "lauxlib.h"
#include "gfx_engine.h"

// Helper: get gray color arg (1–255 int → gfx_color_t)
static gfx_color_t l_color(lua_State *L, int idx)
{
    int c = (int)luaL_checkinteger(L, idx);
    if (c < 0)   c = 0;
    if (c > 255) c = 255;
    return GFX_GRAY(c);
}

static int l_gfx_clear(lua_State *L)
{
    gfx_color_t c = l_color(L, 1);
    gfx_clear(c);
    return 0;
}

static int l_gfx_pixel(lua_State *L)
{
    int x = (int)luaL_checkinteger(L, 1);
    int y = (int)luaL_checkinteger(L, 2);
    gfx_color_t c = l_color(L, 3);
    gfx_pixel(x, y, c);
    return 0;
}

static int l_gfx_line(lua_State *L)
{
    int x1 = (int)luaL_checkinteger(L, 1);
    int y1 = (int)luaL_checkinteger(L, 2);
    int x2 = (int)luaL_checkinteger(L, 3);
    int y2 = (int)luaL_checkinteger(L, 4);
    gfx_color_t c = l_color(L, 5);
    gfx_line(x1, y1, x2, y2, c);
    return 0;
}

static int l_gfx_rect(lua_State *L)
{
    int x = (int)luaL_checkinteger(L, 1);
    int y = (int)luaL_checkinteger(L, 2);
    int w = (int)luaL_checkinteger(L, 3);
    int h = (int)luaL_checkinteger(L, 4);
    gfx_color_t c = l_color(L, 5);
    bool fill = lua_toboolean(L, 6); // optional, defaults to false
    gfx_rect(x, y, w, h, c, fill);
    return 0;
}

static int l_gfx_circle(lua_State *L)
{
    int cx = (int)luaL_checkinteger(L, 1);
    int cy = (int)luaL_checkinteger(L, 2);
    int r  = (int)luaL_checkinteger(L, 3);
    gfx_color_t c = l_color(L, 4);
    bool fill = lua_toboolean(L, 5); // optional
    gfx_circle(cx, cy, r, c, fill);
    return 0;
}

static int l_gfx_text(lua_State *L)
{
    const char *s = luaL_checkstring(L, 1);
    int x = (int)luaL_checkinteger(L, 2);
    int y = (int)luaL_checkinteger(L, 3);
    gfx_color_t c = GFX_BLACK;
    if (lua_gettop(L) >= 4) c = l_color(L, 4);
    gfx_text(s, x, y, c);
    return 0;
}

static int l_gfx_flip(lua_State *L)
{
    (void)L;
    gfx_flush();
    return 0;
}

// Width/height constants accessible from Lua: gfx.W, gfx.H
static const luaL_Reg gfx_funcs[] = {
    {"clear",  l_gfx_clear},
    {"pixel",  l_gfx_pixel},
    {"line",   l_gfx_line},
    {"rect",   l_gfx_rect},
    {"circle", l_gfx_circle},
    {"text",   l_gfx_text},
    {"flip",   l_gfx_flip},
    {NULL, NULL}
};

int mm_lua_open_gfx(lua_State *L)
{
    luaL_newlib(L, gfx_funcs);

    // Constants
    lua_pushinteger(L, GFX_WIDTH);  lua_setfield(L, -2, "W");
    lua_pushinteger(L, GFX_HEIGHT); lua_setfield(L, -2, "H");
    lua_pushinteger(L, 0);          lua_setfield(L, -2, "BLACK");
    lua_pushinteger(L, 255);        lua_setfield(L, -2, "WHITE");

    return 1;
}
