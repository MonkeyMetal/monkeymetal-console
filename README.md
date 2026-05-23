<div align="center">

# 🐒 MonkeyMetal Console

**An open-source 2D black-and-white handheld console**
**一台开源 2D 黑白掌机**

[English](#english) · [中文](#中文)

![status](https://img.shields.io/badge/status-design%20%2B%20M0-orange)
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

We're at **M0 — clean scaffold**. The full design spec (16 sections, ~700 lines)
lives in [`docs/MonkeyMetal-Console-Design.md`](docs/MonkeyMetal-Console-Design.md).

| Milestone | Status |
| --- | --- |
| **M0** Project skeleton, hardware bring-up | ✅ Done |
| **M1** Graphics engine (Bayer dither + sprites) | 🚧 Next |
| **M2** Lua runtime + cart loader | ⏳ |
| **M3** Built-in Snake demo | ⏳ |
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

#### What you'll see (M0 scaffold)
1. MonkeyMetal boot splash (~1.6 s)
2. Reset reason banner
3. Live BOOT-key state — press the on-board BOOT button and the screen updates

That's all M0 does. Everything else (graphics engine, Lua VM, controller pairing,
store) lands in subsequent milestones.

### 📁 Layout

```
monkeymetal-console/
├── README.md                     this file
├── LICENSE                       MIT
├── docs/
│   ├── MonkeyMetal-Console-Design.md   full design spec
│   └── DEVELOPMENT-GUIDE.md            how to contribute / run an AI agent
├── main/                         M0 scaffold (boot + splash + diagnostic)
├── components/
│   ├── port_bsp/                 ST7305 LCD + SDMMC drivers
│   └── wifi_bsp/                 Wi-Fi STA bring-up
├── partitions.csv                8 MB factory + 4 MB FAT
└── sdkconfig.defaults            PSRAM Octal, BSS-in-PSRAM, etc.
```

### 🤝 Contributing

This project is **architecture + product first**. Day-to-day implementation
is delegated to AI agents that follow
[`docs/DEVELOPMENT-GUIDE.md`](docs/DEVELOPMENT-GUIDE.md). Humans focus on
review, playtesting, and shipping issues.

If you want to write a *game* (not the engine itself), wait for M3 — the cart
format and Lua API will be stable enough by then.

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

**M0 — 工程骨架**。完整设计规范(16 节,约 700 行)见
[`docs/MonkeyMetal-Console-Design.md`](docs/MonkeyMetal-Console-Design.md)。

| 里程碑 | 状态 |
| --- | --- |
| **M0** 工程骨架 + 硬件 bring-up | ✅ 完成 |
| **M1** 图形引擎(Bayer 抖动 + 精灵) | 🚧 下一步 |
| **M2** Lua 运行时 + 卡带加载器 | ⏳ |
| **M3** 内置贪吃蛇 demo | ⏳ |
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

#### 当前能看到什么(M0 骨架)
1. MonkeyMetal 开机动画(约 1.6 秒)
2. 复位原因显示
3. BOOT 键实时检测 —— 按板载 BOOT 键,屏幕同步刷新

M0 阶段就这些。图形引擎、Lua VM、蓝牙配对、商店等都在后续里程碑。

### 📁 目录结构

```
monkeymetal-console/
├── README.md                     本文件
├── LICENSE                       MIT
├── docs/
│   ├── MonkeyMetal-Console-Design.md   完整设计规范
│   └── DEVELOPMENT-GUIDE.md            协作/AI 开发指南
├── main/                         M0 骨架(启动 + 启动屏 + 诊断)
├── components/
│   ├── port_bsp/                 ST7305 LCD + SDMMC 驱动
│   └── wifi_bsp/                 WiFi STA 启动
├── partitions.csv                8 MB factory + 4 MB FAT
└── sdkconfig.defaults            PSRAM 八线、BSS 进 PSRAM 等
```

### 🤝 协作方式

我们做**架构 + 产品定义**。具体实现交给遵循
[`docs/DEVELOPMENT-GUIDE.md`](docs/DEVELOPMENT-GUIDE.md)的 AI agent。
人类负责审阅、试玩、提 issue。

想写游戏(不是引擎)?等 M3 完成再来,那时卡带格式和 Lua API 才稳定。

### 📝 许可

MIT。详见 [`LICENSE`](LICENSE)。引擎、示例、工具都是 MIT。
每个游戏卡带在自己目录里带自己的 license。

</details>
