# gnuboy 1.0.4 核心源码 component
#
# 这一层仅包含平台无关的模拟器代码. 所有平台相关接口
# (vid_*, ev_*, sys_*, pcm_*) 都由 gnuboy_sys_esp32idf 实现.

set(GNUBOY_SRCS
    cpu.c
    mem.c
    fastmem.c
    hw.c
    lcd.c
    lcdc.c
    refresh.c
    palette.c
    sound.c
    rtc.c
    events.c
    keytable.c
    loader.c
    save.c
    debug.c
    emu.c
    rccmds.c
    rckeys.c
    rcvars.c
    rcfile.c
    exports.c
    split.c
    path.c
    inflate.c
    miniz_tinfl.c
)

idf_component_register(
    SRCS ${GNUBOY_SRCS}
    INCLUDE_DIRS "."
    PRIV_REQUIRES
        # gnuboy 自身不直接依赖 IDF 组件, 全靠 sys 层桥接.
        # esp_common 提供 EXT_RAM_BSS_ATTR (esp_attr.h).
        log
        esp_common
)

# ---- 关掉对老 C 代码不友好的 warning, 让旧码顺利编译过 ----
# gnuboy 是 C89/K&R 风格, 直接套 ESP-IDF 的 -Werror 会几百条 fatal.
target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wno-error=all
    -Wno-error=implicit-function-declaration
    -Wno-error=incompatible-pointer-types
    -Wno-error=int-conversion
    -Wno-error=unused-variable
    -Wno-error=unused-but-set-variable
    -Wno-error=unused-function
    -Wno-error=unused-label
    -Wno-error=parentheses
    -Wno-error=sign-compare
    -Wno-error=type-limits
    -Wno-error=switch
    -Wno-error=maybe-uninitialized
    -Wno-error=stringop-overflow
    -Wno-error=stringop-truncation
    -Wno-error=array-bounds
    -Wno-error=char-subscripts
    -Wno-error=format
    -Wno-error=format-overflow
    -Wno-error=builtin-declaration-mismatch
    -Wno-error=misleading-indentation
    -Wno-error=cpp
    # 性能 / 行为兼容
    -fno-strict-aliasing
)
