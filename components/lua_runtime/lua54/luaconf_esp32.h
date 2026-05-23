/// @file luaconf_esp32.h
/// @brief ESP32-S3 specific overrides for Lua 5.4 configuration.
/// Included at the top of luaconf.h via a #include guard trick:
/// never include this directly — it is pulled in by our CMakeLists
/// via -include flag.

#pragma once

// ── Float: use single-precision (ESP32-S3 has FPU, no double FPU) ──────────
#define LUA_FLOAT_TYPE  LUA_FLOAT_FLOAT

// ── Integer: 32-bit (matches ESP32-S3 native word size) ────────────────────
// LUA_INT_INT is 32-bit int on Xtensa.  Default is LUA_INT_LONGLONG (64-bit),
// which wastes memory and is slower on this target.
#define LUA_INT_TYPE    LUA_INT_INT

// ── Path: no filesystem paths on bare metal — disable LUA_PATH lookups ──────
#undef  LUA_PATH_DEFAULT
#define LUA_PATH_DEFAULT  ""
#undef  LUA_CPATH_DEFAULT
#define LUA_CPATH_DEFAULT ""

// ── Stack: reduce max stack depth to save RAM (default 15000) ───────────────
#define LUAI_MAXSTACK   2000

// ── No locale support on ESP-IDF ────────────────────────────────────────────
#define lua_getlocaledecpoint()  '.'

// ── Use our custom allocator that routes to PSRAM ───────────────────────────
// (set at runtime via lua_setallocf; no compile-time hook needed)
