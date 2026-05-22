/*
 * gnuboy_sys_esp32idf.h - gnuboy platform port layer (public API).
 *
 * Internally implements every hook declared in gnuboy/sys.h
 * (vid_*, pcm_*, ev_poll, sys_*) so the gnuboy core can link
 * directly into an ESP-IDF project and run on hardware.
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* One-time setup: allocate 16bpp framebuffer in PSRAM, create event queue. */
void gnuboy_sys_init(void);

/* Run emulator main loop. Call gnuboy_sys_init first. Path example
 * "/sdcard/rom.gbc". Never returns. */
void gnuboy_sys_run(const char *rom_path);

/* Input source -> gnuboy event hook.
 *   gnuboy_keycode: K_UP/K_DOWN/K_LEFT/K_RIGHT/'a'/'b'/K_ENTER/K_TAB ...
 *                   (gnuboy default keymap: a=A, b=B, enter=START, tab=SELECT)
 *   pressed: true = press, false = release. */
void gnuboy_sys_post_key(int gnuboy_keycode, bool pressed);

#ifdef __cplusplus
}
#endif
