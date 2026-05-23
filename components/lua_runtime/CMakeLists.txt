idf_component_register(
    SRCS
        # Lua 5.4.7 core
        "lua54/lapi.c"
        "lua54/lauxlib.c"
        "lua54/lbaselib.c"
        "lua54/lcode.c"
        "lua54/lcorolib.c"
        "lua54/lctype.c"
        "lua54/ldblib.c"
        "lua54/ldebug.c"
        "lua54/ldo.c"
        "lua54/ldump.c"
        "lua54/lfunc.c"
        "lua54/lgc.c"
        "lua54/linit.c"
        "lua54/liolib.c"
        "lua54/llex.c"
        "lua54/lmathlib.c"
        "lua54/lmem.c"
        "lua54/loadlib.c"
        "lua54/lobject.c"
        "lua54/lopcodes.c"
        "lua54/loslib.c"
        "lua54/lparser.c"
        "lua54/lstate.c"
        "lua54/lstring.c"
        "lua54/lstrlib.c"
        "lua54/ltable.c"
        "lua54/ltablib.c"
        "lua54/ltm.c"
        "lua54/lundump.c"
        "lua54/lutf8lib.c"
        "lua54/lvm.c"
        "lua54/lzio.c"
        # MonkeyMetal runtime wrapper + bindings
        "mm_lua_runtime.c"
        "mm_lua_gfx.c"
        "mm_lua_input.c"
        "mm_lua_system.c"
    INCLUDE_DIRS
        "."
        "lua54"
    REQUIRES
        gfx_engine
        port_bsp
        fatfs
        esp_timer
        driver
)

# Suppress warnings from Lua's own C code (not our code)
target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wno-implicit-fallthrough
    -Wno-unused-parameter
    -Wno-format
)
