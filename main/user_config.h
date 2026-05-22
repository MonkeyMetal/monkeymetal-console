/*
 * user_config.h
 * gbemu 工程引脚和参数集中宏定义
 *
 * 目标板: 微雪 ESP32-S3-RLCD-4.2 (4.2" RLCD, ST7305, 300x400)
 */
#pragma once

#include <driver/gpio.h>

/* ===========================================================
 * Display - ST7305 1bpp RLCD via SPI3
 * =========================================================== */
#define GBEMU_LCD_W              400      // 横屏使用宽度
#define GBEMU_LCD_H              300      // 横屏使用高度
#define GBEMU_LCD_MOSI_GPIO      GPIO_NUM_12
#define GBEMU_LCD_SCK_GPIO       GPIO_NUM_11
#define GBEMU_LCD_DC_GPIO        GPIO_NUM_5
#define GBEMU_LCD_CS_GPIO        GPIO_NUM_40
#define GBEMU_LCD_RST_GPIO       GPIO_NUM_41
#define GBEMU_LCD_TE_GPIO        GPIO_NUM_6   // 撕裂同步, 暂未使用

/* ===========================================================
 * SD Card (TF) - SDMMC 1-bit, ROMs and save states live here
 * =========================================================== */
#define GBEMU_SD_CLK_GPIO        38
#define GBEMU_SD_CMD_GPIO        21
#define GBEMU_SD_D0_GPIO         39
#define GBEMU_SD_MOUNTPOINT      "/sdcard"

/* ===========================================================
 * I2C bus shared by ES8311 / RTC PCF85063 / SHTC3 (audio etc.)
 * =========================================================== */
#define GBEMU_I2C_SDA_GPIO       GPIO_NUM_13
#define GBEMU_I2C_SCL_GPIO       GPIO_NUM_14

/* ===========================================================
 * GameBoy native resolution (DMG / CGB)
 * =========================================================== */
#define GB_NATIVE_W              160
#define GB_NATIVE_H              144

/* GB 输出到 16bpp 中间帧缓冲, 由 dither 任务转 1bpp 推到 ST7305.
 * 中间缓冲尺寸: 160 * 144 * 2 = 46080 bytes, 放 PSRAM. */
#define GB_FB_BPP                16
#define GB_FB_BYTES              (GB_NATIVE_W * GB_NATIVE_H * GB_FB_BPP / 8)

/* ===========================================================
 * WiFi UDP input service (开发期用)
 * =========================================================== */
#define GBEMU_UDP_INPUT_PORT     8888

/* ===========================================================
 * 默认 ROM 路径 (TF 卡根目录, FAT32, 文件名固定 rom.gb 或 rom.gbc)
 * 后续可改成菜单选择.
 * =========================================================== */
#define GBEMU_DEFAULT_ROM_PATH   "/sdcard/rom.gbc"
