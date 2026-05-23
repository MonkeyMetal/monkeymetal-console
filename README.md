<div align="center">

# 🐒 MonkeyMetal Console

**An open-source 2D black-and-white handheld console**
**一台开源 2D 黑白掌机**

[English](#english) · [中文](#中文)

![status](https://img.shields.io/badge/status-M3%20Snake%20playable-brightgreen)
![license](https://img.shields.io/badge/license-MIT-blue)
![target](https://img.shields.io/badge/board-ESP32--S3--RLCD--4.2-red)
![idf](https://img.shields.io/badge/ESP--IDF-v5.5.x-green)
![input](https://img.shields.io/badge/input-Bluetooth%20HID-blueviolet)

</div>

---

<a id="english"></a>
<details open>
<summary><h2>🇬🇧 English</h2></summary>

### What is this?

**MonkeyMetal Console** is an open-source handheld game console built on the
[Waveshare ESP32-S3-RLCD-4.2](https://www.waveshare.net/wiki/ESP32-S3-RLCD-4.2)
dev board. Think of it as a hackable, sunlight-readable 2D black-and-white
mini console with:

- **Reflective 1bpp LCD** (300×400) — readable outdoors, ultra-low power
- **Bluetooth HID controllers** (8BitDo / Xbox / Switch Pro …)
- **Online game store** — pull free open-source games over Wi-Fi
- **Lua-scripted game runtime** — anyone who knows Lua can ship a game
- **MIT licensed engine** — fork it, mod it, sell it

This is **not an emulator**. It runs games made specifically for this console.

### 🛠️ Hardware

| Item | Spec |
| --- | --- |
| Board | [Waveshare ESP32-S3-RLCD-4.2](https://www.waveshare.net/wiki/ESP32-S3-RLCD-4.2) |
| MCU | ESP32-S3 dual-core Xtensa LX7 @240 MHz |
| RAM | 8 MB Octal PSRAM @80 MHz |
| Flash | 16 MB QIO |
| Display | ST7305 reflective LCD, 400×300, 1bpp |
| Storage | TF/microSD, 1-bit SDMMC |
| Audio | ES8311 codec + I²S |
| Wireless | Wi-Fi 2.4 G + Bluetooth 5 (BLE/Classic) |
| Power | USB-C, optional 3.7 V Li-ion + TP4056 |

### 📊 Project status

We're at **M3 — Snake is playable**. The full design spec (16 sections, ~700 lines)
lives in [`docs/MonkeyMetal-Console-Design.md`](docs/MonkeyMetal-Console-Design.md).

| Milestone | Status |
| --- | --- |
| **M0** Project skeleton, hardware bring-up | ✅ Done |
| **M1** Graphics engine (Bayer dither + draw API) | ✅ Done (hardware verified 2026-05-23) |
| **M2** Lua 5.4 runtime + cart loader | ✅ Done (hardware verified 2026-05-23) |
| **M3** Built-in Snake demo | ✅ Done (hardware verified 2026-05-23) |
| **M4** Audio (ES8311 + 8-channel mixer) | ⏳ |
| **M5** Bluetooth HID controllers | ⏳ |
| **M6** Launcher / system menu | ⏳ |
| **M7** Online store + OTA | ⏳ |

### 🚀 Quick start

#### Prerequisites
- ESP-IDF v5.5.x
- VS Code with the **Espressif IDF** extension (recommended)
- Waveshare ESP32-S3-RLCD-4.2 + USB-C cable
- A FAT32-formatted TF card

#### Build & flash
```bash
git clone https://github.com/MonkeyMetal/monkeymetal-console.git
cd monkeymetal-console
cp components/wifi_bsp/user_secrets.h.example components/wifi_bsp/user_secrets.h
# edit user_secrets.h with your Wi-Fi SSID/password

idf.py set-target esp32s3
idf.py -p COMx build flash monitor
```

In VS Code: `Ctrl+Shift+P` → `ESP-IDF: Build, Flash and Monitor your Project`.

#### What you'll see (M3)
1. MonkeyMetal boot splash (~1.6 s)
2. Snake title screen — press **BOOT** to start
3. Play Snake: **BOOT key = turn right 90°**, eat food to grow, avoid walls and yourself
4. Game Over screen with score bar — press **BOOT** to restart

Controls use the on-board BOOT key only (Bluetooth HID controllers land in M5).

### 📁 Layout

```
monkeymetal-console/
├── README.md                     this file
├── LICENSE                       MIT
├── docs/
│   ├── MonkeyMetal-Console-Design.md   full design spec
│   └── DEVELOPMENT-GUIDE.md            how to contribute / run an AI agent
├── main/                         boot sequence + cart launch
├── components/
│   ├── gfx_engine/               16bpp framebuffer + Bayer dither + draw API
│   ├── lua_runtime/              Lua 5.4 VM + gfx/input/system bindings
│   ├── port_bsp/                 ST7305 LCD + SDMMC drivers
│   └── wifi_bsp/                 Wi-Fi STA bring-up
├── sdcard_root/
│   └── games/
│       ├── hello/                M2 hello world cart
│       └── snake/                M3 Snake game cart
├── docs/
│   ├── MonkeyMetal-Console-Design.md
│   ├── DEVELOPMENT-GUIDE.md
│   └── gfx-engine.md
├── partitions.csv                8 MB factory + 4 MB FAT
└── sdkconfig.defaults            PSRAM Octal, BSS-in-PSRAM, etc.
```

### 🤝 Contributing

This project is **architecture + product first**. Day-to-day implementation
is delegated to AI agents that follow
[`docs/DEVELOPMENT-GUIDE.md`](docs/DEVELOPMENT-GUIDE.md). Humans focus on
review, playtesting, and shipping issues.

**M3 is done — the cart format and Lua API are stable.** If you want to write a
game, copy `sdcard_root/games/snake/` as a template and start hacking `cart.lua`.

### 📝 License

MIT. See [`LICENSE`](LICENSE). Engine code, examples, and tools are all MIT.
Each game cart ships with its own license inside its directory.

</details>

---

<a id="中文"></a>
<details>
<summary><h2>🇨🇳 中文</h2></summary>

### 这是什么

**MonkeyMetal Console** 是一台基于
[微雪 ESP32-S3-RLCD-4.2](https://www.waveshare.net/wiki/ESP32-S3-RLCD-4.2)
开发板的开源掌机。它的卖点:

- **反射式 1bpp LCD**(300×400)—— 阳光下可读、功耗极低
- **蓝牙手柄**(8BitDo / Xbox / Switch Pro 都能用)
- **在线游戏商店** —— 通过 WiFi 下载免费开源游戏
- **Lua 脚本游戏运行时** —— 会写 Lua 就能做游戏
- **MIT 协议** —— 随便 fork、改、商用

它**不是模拟器**,跑的是专门为这台机器写的游戏。

### 🛠️ 硬件规格

| 项目 | 规格 |
| --- | --- |
| 开发板 | [微雪 ESP32-S3-RLCD-4.2](https://www.waveshare.net/wiki/ESP32-S3-RLCD-4.2) |
| 主控 | ESP32-S3 双核 Xtensa LX7 @240 MHz |
| RAM | 8 MB 八线 PSRAM @80 MHz |
| Flash | 16 MB QIO |
| 显示 | ST7305 反射屏,400×300,1bpp |
| 存储 | TF/microSD,1-bit SDMMC |
| 音频 | ES8311 编解码 + I²S |
| 无线 | WiFi 2.4G + 蓝牙 5(BLE/经典) |
| 供电 | USB-C,选配 3.7V 锂电 + TP4056 |

### 📊 项目进度

**M3 — 贪吃蛇可玩**。完整设计规范(16 节,约 700 行)见
[`docs/MonkeyMetal-Console-Design.md`](docs/MonkeyMetal-Console-Design.md)。

| 里程碑 | 状态 |
| --- | --- |
| **M0** 工程骨架 + 硬件 bring-up | ✅ 完成 |
| **M1** 图形引擎(Bayer 抖动 + 绘图 API) | ✅ 完成(真机验收 2026-05-23) |
| **M2** Lua 5.4 运行时 + 卡带加载器 | ✅ 完成(真机验收 2026-05-23) |
| **M3** 内置贪吃蛇 demo | ✅ 完成(真机验收 2026-05-23) |
| **M4** 音频(ES8311 + 8 通道混音) | ⏳ |
| **M5** 蓝牙 HID 手柄 | ⏳ |
| **M6** 系统菜单 | ⏳ |
| **M7** 在线商店 + OTA | ⏳ |

### 🚀 快速上手

#### 准备
- ESP-IDF v5.5.x
- VS Code + **Espressif IDF** 扩展(推荐)
- 微雪 ESP32-S3-RLCD-4.2 + Type-C 线
- FAT32 格式化的 TF 卡

#### 编译烧录
```bash
git clone https://github.com/MonkeyMetal/monkeymetal-console.git
cd monkeymetal-console
cp components/wifi_bsp/user_secrets.h.example components/wifi_bsp/user_secrets.h
# 编辑 user_secrets.h 填 WiFi 凭据

idf.py set-target esp32s3
idf.py -p COMx build flash monitor
```

VS Code: `Ctrl+Shift+P` → `ESP-IDF: Build, Flash and Monitor your Project`。

#### 当前能看到什么(M3)
1. MonkeyMetal 开机动画(约 1.6 秒)
2. 贪吃蛇标题画面 —— 按 **BOOT 键** 开始
3. 游戏:按 **BOOT 键右转 90°**,吃食物变长,碰墙或碰自身结束
4. Game Over 画面 + 得分条 —— 按 **BOOT 键** 重开

目前输入只有板载 BOOT 键(M5 蓝牙手柄后会换成真正的手柄操作)。

### 📁 目录结构

```
monkeymetal-console/
├── README.md                     本文件
├── LICENSE                       MIT
├── docs/
│   ├── MonkeyMetal-Console-Design.md   完整设计规范
│   └── DEVELOPMENT-GUIDE.md            协作/AI 开发指南
├── main/                         启动序列 + 加载卡带
├── components/
│   ├── gfx_engine/               16bpp 帧缓冲 + Bayer 抖动 + 绘图 API
│   ├── lua_runtime/              Lua 5.4 VM + gfx/input/system binding
│   ├── port_bsp/                 ST7305 LCD + SDMMC 驱动
│   └── wifi_bsp/                 WiFi STA 启动
├── sdcard_root/
│   └── games/
│       ├── hello/                M2 hello world 卡带
│       └── snake/                M3 贪吃蛇卡带
├── docs/
│   ├── MonkeyMetal-Console-Design.md   完整设计规范
│   ├── DEVELOPMENT-GUIDE.md            协作/AI 开发指南
│   └── gfx-engine.md                   图形引擎文档
├── partitions.csv                8 MB factory + 4 MB FAT
└── sdkconfig.defaults            PSRAM 八线、BSS 进 PSRAM 等
```

### 🤝 协作方式

我们做**架构 + 产品定义**。具体实现交给遵循
[`docs/DEVELOPMENT-GUIDE.md`](docs/DEVELOPMENT-GUIDE.md)的 AI agent。
人类负责审阅、试玩、提 issue。

**M3 已完成,卡带格式和 Lua API 已稳定。** 想写游戏?把 `sdcard_root/games/snake/` 拷一份,改 `cart.lua` 就能跑。

### 📝 许可

MIT。详见 [`LICENSE`](LICENSE)。引擎、示例、工具都是 MIT。
每个游戏卡带在自己目录里带自己的 license。

</details>
