/*
 * monkey_metal.h - Boot splash "MonkeyMetal" for gbemu.
 *
 * Pure 1bpp pixel art rendered straight to ST7305 - no fonts, no PSRAM,
 * just direct RLCD_SetPixel calls. Animation: monkey blinks, banner slides
 * in, total ~1.6 seconds before the emulator takes over.
 *
 * Why a splash: gives instant visual confirmation that LCD + boot path
 * works, especially helpful while the SD mount + ROM load can take a
 * second on slow cards.
 */
#pragma once

#include "display_bsp.h"

#ifdef __cplusplus
extern "C" {
#endif

void monkey_metal_play(DisplayPort *lcd);

#ifdef __cplusplus
}
#endif
