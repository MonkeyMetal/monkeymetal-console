# gbemu

> A GameBoy / GameBoy Color emulator running on the **Waveshare ESP32-S3-RLCD-4.2**
> reflective-LCD AIoT board.
>
> 在微雪 **ESP32-S3-RLCD-4.2** 反射屏 AIoT 开发板上跑 GameBoy / GameBoy Color 游戏。

---

## English

### What is this?

A from-scratch port of the [gnuboy](http://gnuboy.unix-fu.org/) GameBoy emulator
to the ESP32-S3-RLCD-4.2 board, with the goal of turning the dev board into a
playable handheld console.

The board has an unusual **300×400 1-bit-per-pixel reflective LCD** (ST7305) — no
backlight, sunlight readable, but only black and white. This project converts
GameBoy's 4-shade output to 1-bit using Bayer 4×4 dithering.

### Hardware

| Item | Detail |
| --- | --- |
| Board | Waveshare ESP32-S3-RLCD-4.2 |
| MCU | ESP32-S3 dual-core @240MHz, 8 MB Octal PSRAM, 16 MB flash |
| Display | ST7305 reflective LCD, 400×300, 1bpp |
| Storage | TF / microSD card (1-bit SDMMC) |
| ROM source | `/sdcard/rom.gbc` (raw `.gb` or `.gbc`, FAT32 root) |

### Status

| Subsystem | State |
| --- | --- |
| LCD bring-up (ST7305) | ✅ |
| SD card mount + ROM read | ✅ |
| WiFi STA | ✅ |
| Boot splash + BOOT-key diagnostic | ✅ |
| gnuboy core compiles + links | ✅ |
| gnuboy `loader_init` parses ROM header | ✅ |
| `cpu_emulate` runs first instruction | ❌ — `rom.bank` pointer corrupted, under investigation |
| Bayer 4×4 dithering | ⏳ planned |
| WiFi UDP keypad input | ⏳ planned |
| Hardware 8-key board (DPad+AB+START/SELECT) | ⏳ planned |
| ES8311 audio | ⏳ planned |

### Build & run

Requires **ESP-IDF v5.5.x**.

```bash
# Activate IDF environment first, then:
cd gbemu
idf.py set-target esp32s3
idf.py build flash monitor
```

In VS Code with the Espressif IDF extension:
`Ctrl+Shift+P` → `ESP-IDF: Build, Flash and Monitor your Project`.

### WiFi credentials

WiFi SSID/password live in a **gitignored** header.
Copy the template, fill in your network:

```bash
cp components/wifi_bsp/user_secrets.h.example components/wifi_bsp/user_secrets.h
# then edit the new file
```

### Project layout

```
gbemu/
├── main/
│   ├── main.cpp                     boot sequence (NVS → LCD → SD → WiFi → emu)
│   ├── monkey_metal.cpp/h           "MonkeyMetal" boot splash animation
│   ├── key_test.cpp/h               diagnostic: shows reset reason + BOOT key state
│   └── user_config.h                GPIO pin map
├── components/
│   ├── port_bsp/                    ST7305 LCD + SDMMC drivers (from Waveshare demo)
│   ├── wifi_bsp/                    WiFi STA bring-up
│   ├── gnuboy/                      gnuboy 1.0.4 emulator core (GPLv2)
│   └── gnuboy_sys_esp32idf/         platform port: vid_/pcm_/ev_/sys_*
├── partitions.csv                   8 MB factory + 4 MB FAT storage
└── sdkconfig.defaults               PSRAM Octal @80MHz, BSS to PSRAM, etc.
```

### About ROMs

This repository contains **no game ROMs**. You provide your own `rom.gbc` /
`rom.gb` (legally, from a cartridge you own — for example via a GB Operator
dump). Place it on the TF card root.

### License

GPL-2.0-or-later. The bulk of the code is from gnuboy 1.0.4, which is GPLv2 —
see `LICENSE`.

### Acknowledgements

- [gnuboy](http://gnuboy.unix-fu.org/) — Laguna et al.
- [Waveshare](https://www.waveshare.net/wiki/ESP32-S3-RLCD-4.2) — board + demo code
- [Espressif](https://www.espressif.com/) — ESP-IDF toolchain

---

## 中文

### 这是什么

把 [gnuboy](http://gnuboy.unix-fu.org/) GameBoy 模拟器从零移植到微雪
ESP32-S3-RLCD-4.2 开发板,目标是把这块板做成能玩 GB/GBC 游戏的掌机。

这块板有个特别的 **300×400 1bpp 反射屏**(ST7305 驱动),不需要背光、阳光下也看得清,
缺点是只有黑白两色。本项目用 Bayer 4×4 抖动算法把 GB 的 4 阶灰度压成 1 位。

### 硬件

| 项目 | 详情 |
| --- | --- |
| 开发板 | 微雪 ESP32-S3-RLCD-4.2 |
| 主控 | ESP32-S3 双核 @240MHz,8 MB Octal PSRAM,16 MB Flash |
| 显示 | ST7305 反射屏,400×300,1bpp |
| 存储 | TF/microSD 卡(1-bit SDMMC 模式) |
| ROM 来源 | `/sdcard/rom.gbc`(裸 `.gb` 或 `.gbc`,FAT32 根目录) |

### 当前状态

| 子系统 | 状态 |
| --- | --- |
| LCD 驱动(ST7305) | ✅ |
| SD 卡挂载 + 读 ROM | ✅ |
| WiFi STA 联网 | ✅ |
| 开机动画 + BOOT 键诊断模式 | ✅ |
| gnuboy 核心编译 + 链接 | ✅ |
| gnuboy `loader_init` 解析 ROM 头 | ✅ |
| `cpu_emulate` 跑第一条指令 | ❌ —— `rom.bank` 指针损坏,排查中 |
| Bayer 4×4 抖动 | ⏳ 待做 |
| WiFi UDP 手柄输入 | ⏳ 待做 |
| 物理 8 键面板(↑↓←→AB START SELECT) | ⏳ 待做 |
| ES8311 音频输出 | ⏳ 待做 |

### 编译与烧录

需要 **ESP-IDF v5.5.x**。

```bash
# 进 IDF 环境后:
cd gbemu
idf.py set-target esp32s3
idf.py build flash monitor
```

VS Code 装了 Espressif IDF 扩展的话:
`Ctrl+Shift+P` → `ESP-IDF: Build, Flash and Monitor your Project`。

### WiFi 凭据

SSID 和密码放在一个**已加 .gitignore 的头文件**里。复制模板再填入自己的网络:

```bash
cp components/wifi_bsp/user_secrets.h.example components/wifi_bsp/user_secrets.h
# 再编辑这个新文件
```

### 工程结构

```
gbemu/
├── main/
│   ├── main.cpp                     启动流程(NVS → LCD → SD → WiFi → 模拟器)
│   ├── monkey_metal.cpp/h           "MonkeyMetal" 开机动画
│   ├── key_test.cpp/h               诊断模式: 显示复位原因 + BOOT 键实时状态
│   └── user_config.h                GPIO 引脚集中宏
├── components/
│   ├── port_bsp/                    ST7305 LCD + SDMMC 驱动(改自微雪 demo)
│   ├── wifi_bsp/                    WiFi STA 启动
│   ├── gnuboy/                      gnuboy 1.0.4 模拟器核心(GPLv2)
│   └── gnuboy_sys_esp32idf/         平台移植层: vid_/pcm_/ev_/sys_*
├── partitions.csv                   8MB 应用 + 4MB FAT 存储分区
└── sdkconfig.defaults               PSRAM Octal @80MHz,.bss 放 PSRAM 等
```

### 关于 ROM

仓库**不包含任何游戏 ROM**。请自备 `rom.gbc` / `rom.gb`(合法途径,比如用 GB
Operator 从你拥有的实体卡带里 dump)。放到 TF 卡根目录。

### 许可

GPL-2.0-or-later。大部分代码来自 gnuboy 1.0.4(GPLv2),详见 `LICENSE`。

### 致谢

- [gnuboy](http://gnuboy.unix-fu.org/) —— Laguna 等人
- [微雪 Waveshare](https://www.waveshare.net/wiki/ESP32-S3-RLCD-4.2) —— 开发板 + demo 代码
- [乐鑫 Espressif](https://www.espressif.com/) —— ESP-IDF 工具链
