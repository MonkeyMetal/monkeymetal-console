/*
 * gnuboy_sys_esp32idf.h - gnuboy 平台移植层对外接口.
 *
 * 这一层把 gnuboy 核心和具体硬件解耦. main.cpp 只调用这里的 init,
 * 后续 emu_run 主循环会在内部调用 gnuboy 的 emu_init/loader_init/emu_run.
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* 初始化 dither 任务 / 屏幕推送 / 输入子系统 (后期会扩展). 当前为占位. */
void gnuboy_sys_init(void);

/* 启动模拟主循环, 加载指定 ROM. 成功不返回. 失败 log + 返回. */
void gnuboy_sys_run(const char *rom_path);

#ifdef __cplusplus
}
#endif
