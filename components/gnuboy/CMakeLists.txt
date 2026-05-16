# gnuboy 核心源码占位.
#
# 任务 #9 会把 D:/xxw_p/wx_ESP32-S3/相关资料/gnuboy-master/ 下的核心 .c 文件
# (cpu.c lcd.c lcdc.c refresh.c mem.c fastmem.c hw.c loader.c sound.c
#  newsound.c save.c rtc.c events.c palette.c rcvars.c rccmds.c rcfile.c
#  rckeys.c keytable.c 等) 拷贝进来, 同时删除 dummy.c.
idf_component_register(
    SRCS
        "dummy.c"
    INCLUDE_DIRS
        "."
)
