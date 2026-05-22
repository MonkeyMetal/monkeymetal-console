<div align="center">

# 🎮 gbemu

**A GameBoy / GameBoy Color emulator for the Waveshare ESP32-S3-RLCD-4.2 reflective LCD board**

**让微雪 ESP32-S3-RLCD-4.2 反射屏开发板变身 GameBoy 掌机**

[English](#english) · [中文](#中文)

![status](https://img.shields.io/badge/status-WIP-orange)
![license](https://img.shields.io/badge/license-GPL--2.0-blue)
![target](https://img.shields.io/badge/target-ESP32--S3-red)
![idf](https://img.shields.io/badge/ESP--IDF-v5.5.x-green)

</div>

---

<a id="english"></a>
<details open>
<summary><h2>🇬🇧 English</h2></summary>

### What is this?

A from-scratch port of the [gnuboy](http://gnuboy.unix-fu.org/) GameBoy emulator
to the **Waveshare ESP32-S3-RLCD-4.2** development board. The goal is to turn
this dev board into a battery-friendly handheld console with a sunlight-readable
black-and-white screen.

The board's signature feature is a **300×400 1-bit-per-pixel reflective LCD**
(ST7305) — no backlight, readable in direct sunlight, very low power. We use
**Bayer 4×4 dithering** to convert GameBoy's 4 grayscale levels into pure
black-and-white pixels.

### 🛠️ Hardware

| Item | Spec |
| --- | --- |
| Board | [Waveshare ESP32-S3-RLCD-4.2](https://www.waveshare.net/wiki/ESP32-S3-RLCD-4.2) |
| MCU | ESP32-S3, dual-core Xtensa LX7 @240 MHz |
| RAM | 8 MB Octal PSRAM @80 MHz |
| Flash | 16 MB QIO |
| Display | ST7305 reflective LCD, 400×300, 1bpp |
| SD slot | TF / microSD, 1-bit SDMMC mode |
| Audio | ES8311 codec (planned) |
| Sensors | PCF85063 RTC, SHTC3 (not used) |
| USB | Type-C, with built-in USB-JTAG (no programmer needed) |

### 📌 Pinout (used by gbemu)

| Function | GPIO |
| --- | --- |
| LCD MOSI | 12 |
| LCD SCK | 11 |
| LCD DC | 5 |
| LCD CS | 40 |
| LCD RST | 41 |
| SD CLK | 38 |
| SD CMD | 21 |
| SD D0 | 39 |
| I²C SDA | 13 |
| I²C SCL | 14 |
| BOOT button | 0 |

### 📊 Current progress

| Subsystem | Status |
| --- | --- |
| ST7305 LCD driver | ✅ Working |
| SD card mount + ROM read | ✅ Working |
| WiFi STA connect | ✅ Working |
| MonkeyMetal boot splash | ✅ Working |
| Reset reason + BOOT key diagnostic | ✅ Working |
| gnuboy core compile + link | ✅ Working |
| `loader_init` parses ROM header | ✅ Working |
| `cpu_emulate` first instruction | ❌ `rom.bank` corrupted, debugging |
| Bayer 4×4 dithering | ⏳ Planned |
| WiFi UDP keypad input | ⏳ Planned |
| Hardware 8-key board (DPad+AB+START/SELECT) | ⏳ Planned |
| ES8311 audio | ⏳ Planned |

### 🚀 Getting started

#### 1. Prerequisites

- **ESP-IDF v5.5.x** installed ([Espressif install guide](https://docs.espressif.com/projects/esp-idf/en/v5.5/esp32s3/get-started/index.html))
- **VS Code** + **Espressif IDF extension** (recommended) or any environment with `idf.py` available
- A **Waveshare ESP32-S3-RLCD-4.2** board + USB-C cable
- A **TF/microSD card** (FAT32 formatted) and a card reader
- A legally-obtained `.gb` or `.gbc` ROM file

#### 2. Clone

```bash
git clone https://github.com/MonkeyMetal/gbemu.git
cd gbemu
```

#### 3. Configure WiFi credentials

```bash
cp components/wifi_bsp/user_secrets.h.example components/wifi_bsp/user_secrets.h
```

Then edit `components/wifi_bsp/user_secrets.h`:

```c
#define GBEMU_WIFI_SSID     "your_wifi_name"
#define GBEMU_WIFI_PASSWORD "your_wifi_password"
```

This file is in `.gitignore`, so your real credentials never get committed.

#### 4. Prepare the SD card

1. Format the TF card as **FAT32**
2. Copy your ROM to the **root** of the card
3. Rename it to **`rom.gbc`** (lowercase, even for `.gb` files)
4. Safely eject and insert into the board's TF slot

#### 5. Build & flash

VS Code:

> `Ctrl+Shift+P` → `ESP-IDF: Build, Flash and Monitor your Project` → 🔥

Command line:

```bash
idf.py set-target esp32s3
idf.py -p COMx build flash monitor    # Linux/macOS: -p /dev/ttyUSBx
```

First build pulls in WiFi/lwIP and takes 3–8 minutes; later builds are fast.

#### 6. What you should see

1. **Boot splash**: a monkey ("MonkeyMetal") with the title beneath, ~1.6 s
2. **Diagnostic mode** (current build): shows reset reason, then watches the
   BOOT key — press it and the screen shows `BOOT` / `PRESSED`
3. Game loading is **not enabled** in this build because of an outstanding
   `rom.bank` corruption bug. To switch back to emulator mode, edit
   `main/main.cpp` and replace `key_test_run(&g_lcd)` with
   `gnuboy_sys_run(GBEMU_DEFAULT_ROM_PATH)` — but it will currently crash on
   the first CPU instruction.

### 📁 Project layout

```
gbemu/
├── main/
│   ├── main.cpp                  Boot sequence (NVS → LCD → SD → WiFi → emu)
│   ├── monkey_metal.cpp/h        Boot splash
│   ├── key_test.cpp/h            Reset-reason + BOOT-key diagnostic
│   └── user_config.h             GPIO pin map (one place)
├── components/
│   ├── port_bsp/                 ST7305 LCD + SDMMC drivers (from Waveshare demo)
│   ├── wifi_bsp/                 WiFi STA bring-up
│   ├── gnuboy/                   gnuboy 1.0.4 emulator core (GPLv2)
│   └── gnuboy_sys_esp32idf/      Platform port: vid_/pcm_/ev_/sys_*
├── partitions.csv                8 MB factory + 4 MB FAT
└── sdkconfig.defaults            PSRAM Octal @80MHz, BSS-in-PSRAM, etc.
```

### ⚠️ About ROMs

This repository **does not include any game ROM**. You are responsible for
obtaining ROMs legally — typically by dumping cartridges you own using a tool
like GB Operator, or using homebrew/CC0-licensed games.

### 📝 License

**GPL-2.0-or-later**. Most of the code comes from gnuboy 1.0.4 which is GPLv2,
so the whole project must follow the same license. See `LICENSE`.

### 🙏 Acknowledgements

- [gnuboy](http://gnuboy.unix-fu.org/) — Laguna et al.
- [Waveshare](https://www.waveshare.net/wiki/ESP32-S3-RLCD-4.2) — board + demo code
- [Espressif](https://www.espressif.com/) — ESP-IDF toolchain

</details>

---

<a id="中文"></a>
<details>
<summary><h2>🇨🇳 中文</h2></summary>

### 这是什么

把 [gnuboy](http://gnuboy.unix-fu.org/) GameBoy 模拟器从零移植到**微雪
ESP32-S3-RLCD-4.2** 开发板。目标是把这块板做成一台续航好、阳光下也能看清的
黑白掌机。

板子的最大特色是一块 **300×400 1bpp 反射屏**(ST7305 驱动)——不需要背光、太阳
底下也清楚、功耗极低,但只有黑白两色。我们用 **Bayer 4×4 抖动算法**把 GB 的
4 阶灰度压成纯黑白像素。

### 🛠️ 硬件

| 项目 | 规格 |
| --- | --- |
| 开发板 | [微雪 ESP32-S3-RLCD-4.2](https://www.waveshare.net/wiki/ESP32-S3-RLCD-4.2) |
| 主控 | ESP32-S3,双核 Xtensa LX7 @240 MHz |
| RAM | 8 MB 八线 PSRAM @80 MHz |
| Flash | 16 MB QIO |
| 显示 | ST7305 反射屏,400×300,1bpp |
| 卡槽 | TF / microSD,1-bit SDMMC 模式 |
| 音频 | ES8311 编解码芯片(暂未用) |
| 传感器 | PCF85063 RTC、SHTC3(暂未用) |
| USB | Type-C,内置 USB-JTAG(不需要外置烧录器) |

### 📌 引脚分配(gbemu 用到的)

| 功能 | GPIO |
| --- | --- |
| LCD MOSI | 12 |
| LCD SCK | 11 |
| LCD DC | 5 |
| LCD CS | 40 |
| LCD RST | 41 |
| SD CLK | 38 |
| SD CMD | 21 |
| SD D0 | 39 |
| I²C SDA | 13 |
| I²C SCL | 14 |
| BOOT 键 | 0 |

### 📊 当前进度

| 子系统 | 状态 |
| --- | --- |
| ST7305 LCD 驱动 | ✅ 完成 |
| SD 卡挂载 + ROM 读取 | ✅ 完成 |
| WiFi STA 联网 | ✅ 完成 |
| MonkeyMetal 开机动画 | ✅ 完成 |
| 复位原因 + BOOT 键诊断 | ✅ 完成 |
| gnuboy 核心编译 + 链接 | ✅ 完成 |
| `loader_init` 解析 ROM 头 | ✅ 完成 |
| `cpu_emulate` 跑第一条指令 | ❌ `rom.bank` 损坏,排查中 |
| Bayer 4×4 抖动 | ⏳ 待做 |
| WiFi UDP 手柄输入 | ⏳ 待做 |
| 物理 8 键面板(↑↓←→AB START SELECT) | ⏳ 待做 |
| ES8311 音频 | ⏳ 待做 |

### 🚀 上手教程

#### 1. 先准备

- **ESP-IDF v5.5.x** 已安装([乐鑫安装指南](https://docs.espressif.com/projects/esp-idf/zh_CN/v5.5/esp32s3/get-started/index.html))
- **VS Code** + **Espressif IDF 扩展**(推荐),或任意能用 `idf.py` 的环境
- 一块 **微雪 ESP32-S3-RLCD-4.2** 板 + USB-C 数据线
- 一张 **TF/microSD 卡**(FAT32 格式) + 读卡器
- 一个**合法获得**的 `.gb` 或 `.gbc` ROM 文件

#### 2. 拉代码

```bash
git clone https://github.com/MonkeyMetal/gbemu.git
cd gbemu
```

#### 3. 配置 WiFi

```bash
cp components/wifi_bsp/user_secrets.h.example components/wifi_bsp/user_secrets.h
```

然后编辑 `components/wifi_bsp/user_secrets.h`:

```c
#define GBEMU_WIFI_SSID     "你的wifi名称"
#define GBEMU_WIFI_PASSWORD "你的wifi密码"
```

这个文件已在 `.gitignore` 里,真实凭据不会被 git 提交。

#### 4. 准备 SD 卡

1. TF 卡格式化成 **FAT32**
2. ROM 文件拷到卡的**根目录**
3. 改名为 **`rom.gbc`**(全小写,即使是 `.gb` 文件也用这个名字)
4. 安全弹出,插到板子的 TF 卡槽

#### 5. 编译烧录

VS Code:

> `Ctrl+Shift+P` → `ESP-IDF: Build, Flash and Monitor your Project` → 🔥

命令行:

```bash
idf.py set-target esp32s3
idf.py -p COMx build flash monitor    # Linux/macOS: -p /dev/ttyUSBx
```

首次编译要拉 WiFi/lwIP 等组件,耗时 3–8 分钟,之后增量编译很快。

#### 6. 应该看到什么

1. **开机动画**:一只猴子(MonkeyMetal)+ 下方标题,约 1.6 秒
2. **诊断模式**(当前 build):先显示复位原因,然后监听 BOOT 键 —— 按下屏幕变
   `BOOT` / `PRESSED`
3. 游戏运行**当前已禁用**,因为还有一个 `rom.bank` 损坏 bug。想切回模拟器
   模式,编辑 `main/main.cpp` 把 `key_test_run(&g_lcd)` 换成
   `gnuboy_sys_run(GBEMU_DEFAULT_ROM_PATH)` —— 但目前 CPU 第一条指令就会崩。

### 📁 工程结构

```
gbemu/
├── main/
│   ├── main.cpp                  启动流程(NVS → LCD → SD → WiFi → 模拟器)
│   ├── monkey_metal.cpp/h        开机动画
│   ├── key_test.cpp/h            复位原因 + BOOT 键诊断
│   └── user_config.h             GPIO 引脚集中宏
├── components/
│   ├── port_bsp/                 ST7305 LCD + SDMMC 驱动(改自微雪 demo)
│   ├── wifi_bsp/                 WiFi STA 启动
│   ├── gnuboy/                   gnuboy 1.0.4 模拟器核心(GPLv2)
│   └── gnuboy_sys_esp32idf/      平台移植层: vid_/pcm_/ev_/sys_*
├── partitions.csv                8 MB 应用 + 4 MB FAT
└── sdkconfig.defaults            PSRAM Octal @80MHz、.bss 放 PSRAM 等
```

### ⚠️ 关于 ROM

仓库**不包含任何游戏 ROM**。请自行**合法获取** —— 比如用 GB Operator 从你
拥有的实体卡带里 dump,或使用 homebrew / CC0 授权的开源游戏。

### 📝 许可

**GPL-2.0-or-later**。大部分代码源自 gnuboy 1.0.4(GPLv2),整体必须使用相同
许可。详见 `LICENSE`。

### 🙏 致谢

- [gnuboy](http://gnuboy.unix-fu.org/) —— Laguna 等人
- [微雪 Waveshare](https://www.waveshare.net/wiki/ESP32-S3-RLCD-4.2) —— 开发板 + demo 代码
- [乐鑫 Espressif](https://www.espressif.com/) —— ESP-IDF 工具链

</details>

---

<div align="center">

Made with ❤️ on a Waveshare ESP32-S3-RLCD-4.2 · By **MonkeyMetal**

</div>
