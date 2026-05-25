<div align="center">

# 🐒 MonkeyMetal Console

**An open-source 2D black-and-white handheld console**
**一台开源 2D 黑白掌机**

[English](#english) · [中文](#中文)

![status](https://img.shields.io/badge/version-v1.0.1-brightgreen)
![license](https://img.shields.io/badge/license-MIT-blue)
![target](https://img.shields.io/badge/board-ESP32--S3--RLCD--4.2-red)
![idf](https://img.shields.io/badge/ESP--IDF-v5.5.x-green)
![input](https://img.shields.io/badge/input-Bluetooth%20HID-blueviolet)
![audio](https://img.shields.io/badge/audio-ES8311%20I²S-orange)

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
- **Lua-scripted game runtime** — anyone who knows Lua can ship a game
- **ES8311 audio** — I²S codec with tone/sfx support
- **System launcher** — browse and launch games from TF card
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
| Wireless | Wi-Fi 2.4 G + Bluetooth 5 (BLE) |
| Power | USB-C, optional 3.7 V Li-ion + TP4056 |

### 📊 v1.0.1 features

| Feature | Status |
| --- | --- |
| Graphics engine (16bpp framebuffer + Bayer 4×4 dither + draw API + text) | ✅ |
| Lua 5.4 runtime + cart loader (sandboxed VM, PSRAM allocator) | ✅ |
| Bluetooth HID gamepad (8BitDo / Xbox / Switch Pro, auto-reconnect, fast pairing) | ✅ |
| Audio engine (ES8311 I²S codec, tone/beep API) | ✅ |
| System launcher (browse & launch carts, settings, boot animation) | ✅ |
| **Starfield screensaver** (3D warp particles, auto-activate after idle timeout) | ✅ v1.0.1 |
| **SHTC3 temperature + humidity** (I2C sensor, calibrated) | ✅ v1.0.1 |
| **PCF85063 RTC + SNTP time sync** (network time via WiFi) | ✅ v1.0.1 |
| **WiFi file server** (port 9999, send files over network to TF card) | ✅ v1.0.1 |
| **Settings page** (volume, screensaver timeout, WiFi, IP display) | ✅ v1.0.1 |
| Snake game (D-Pad + A button, eat/die sounds, score tracking) | ✅ |
| Gamepad test cart (visual button/joystick tester) | ✅ |
| BOOT key fallback (GPIO 0, long-press back to launcher) | ✅ |

### 🚀 Quick start

#### Prerequisites
- ESP-IDF v5.5.x
- VS Code with the **Espressif IDF** extension (recommended)
- Waveshare ESP32-S3-RLCD-4.2 + USB-C cable
- A FAT32-formatted TF card
- A Bluetooth gamepad (8BitDo / Xbox / Switch Pro)

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

#### Copy games to TF card
Copy the entire `sdcard_root/` contents to the root of your FAT32 TF card:
```
TF card root/
├── games/
│   ├── snake/
│   │   ├── cart.lua
│   │   └── meta.json
│   └── gamepad_test/
│       └── cart.lua
└── system/
    └── launcher/
        └── cart.lua
```

#### What you'll see
1. **Boot animation** — hyper-drive starfield + system checklist + retro scanline logo
2. **System launcher** — shows available games on TF card
3. **Settings** (SELECT) — volume, screensaver timeout (5-60s), WiFi status, IP address
4. **Starfield screensaver** — 3D warp particles, temperature, humidity, time, IP; auto-activates after idle
5. **Pair a Bluetooth gamepad** — quick 15s scan, auto-connect, BOOT key to skip
6. **Snake**: D-Pad to steer, A to start/restart, eat food to grow, audio feedback
7. **Gamepad Test**: visual feedback for all buttons and joysticks
8. **WiFi file server** — send files to TF card over network (port 9999, `tools/send_file.py`)
9. **BOOT key** — long-press to return to launcher; **SELECT** — instant return

### 📁 Layout

```
monkeymetal-console/
├── README.md
├── LICENSE                       MIT
├── docs/
│   ├── DEVELOPMENT-GUIDE.md      how to contribute / AI agent rules
│   ├── gfx-engine.md             graphics engine reference
│   └── boot-splash.md            boot animation design
├── main/                         boot sequence + cart launch
├── components/
│   ├── gfx_engine/               16bpp framebuffer + Bayer dither + draw API + 8x8 font
│   ├── lua_runtime/              Lua 5.4 VM + gfx/input/system/audio/sensor bindings
│   ├── audio_engine/             ES8311 I²S codec driver + tone API
│   ├── input_bt_hid/             BLE HID gamepad host + auto-reconnect
│   ├── port_bsp/                 ST7305 LCD (factory-calibrated VCOM) + SDMMC drivers
│   └── wifi_bsp/                 Wi-Fi STA + TCP file server (port 9999)
├── tools/
│   └── send_file.py              WiFi file transfer script (PC → console)
├── sdcard_root/
│   ├── games/
│   │   ├── snake/                Snake game cart
│   │   ├── gamepad_test/         Gamepad tester cart
│   │   ├── monkey_leaper/        Monkey Leaper platformer
│   │   └── space_defender/       Space Defender shooter
│   └── system/
│       └── launcher/             System launcher (boot anim + screensaver + settings)
├── partitions.csv                8 MB factory + 4 MB FAT
└── sdkconfig.defaults            PSRAM Octal, BSS-in-PSRAM, etc.
```

### 🤝 Contributing

This project is **architecture + product first**. Day-to-day implementation
is delegated to AI agents that follow
[`docs/DEVELOPMENT-GUIDE.md`](docs/DEVELOPMENT-GUIDE.md). Humans focus on
review, playtesting, and shipping issues.

**The cart format and Lua API are stable.** If you want to write a game, copy
`sdcard_root/games/snake/` as a template and start hacking `cart.lua`.

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
- **Lua 脚本游戏运行时** —— 会写 Lua 就能做游戏
- **ES8311 音频** —— I²S 编解码器,支持音效
- **系统启动器** —— 从 TF 卡浏览并启动游戏
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
| 无线 | WiFi 2.4G + 蓝牙 5(BLE) |
| 供电 | USB-C,选配 3.7V 锂电 + TP4056 |

### 📊 v1.0.1 功能

| 功能 | 状态 |
| --- | --- |
| 图形引擎(16bpp 帧缓冲 + Bayer 4×4 抖动 + 绘图 API + 文字) | ✅ |
| Lua 5.4 运行时 + 卡带加载器(沙箱 VM,PSRAM 分配器) | ✅ |
| 蓝牙 HID 手柄(8BitDo / Xbox / Switch Pro,自动重连,快速配对) | ✅ |
| 音频引擎(ES8311 I²S 编解码,tone/beep API) | ✅ |
| 系统启动器(浏览启动卡带,设置页,开机动画) | ✅ |
| **星空屏保**(3D 穿梭粒子,闲置超时自动激活) | ✅ v1.0.1 |
| **SHTC3 温湿度传感器**(I2C 读取,已校准) | ✅ v1.0.1 |
| **PCF85063 RTC + SNTP 网络校时**(WiFi 连接自动同步) | ✅ v1.0.1 |
| **WiFi 文件服务器**(端口 9999,网络直传文件到 TF 卡) | ✅ v1.0.1 |
| **设置页面**(音量,屏保超时,WiFi 状态,IP 显示) | ✅ v1.0.1 |
| 贪吃蛇游戏(方向键 + A 键,吃/死音效,分数记录) | ✅ |
| 手柄测试卡带(可视化按键/摇杆测试) | ✅ |
| BOOT 键备用(GPIO 0,长按返回启动器) | ✅ |

### 🚀 快速上手

#### 准备
- ESP-IDF v5.5.x
- VS Code + **Espressif IDF** 扩展(推荐)
- 微雪 ESP32-S3-RLCD-4.2 + Type-C 线
- FAT32 格式化的 TF 卡
- 蓝牙手柄(8BitDo / Xbox / Switch Pro)

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

#### 把游戏拷到 TF 卡
把 `sdcard_root/` 目录下所有内容拷到 FAT32 TF 卡根目录:
```
TF 卡根目录/
├── games/
│   ├── snake/
│   │   ├── cart.lua
│   │   └── meta.json
│   └── gamepad_test/
│       └── cart.lua
└── system/
    └── launcher/
        └── cart.lua
```

#### 当前能看到什么
1. **开机动画** —— 超光速星空 + 系统自检列表 + 复古扫描线 Logo
2. **系统启动器** —— 显示 TF 卡上的可用游戏
3. **设置页面**(SELECT) —— 音量,屏保超时(5-60秒),WiFi 状态,IP 地址
4. **星空屏保** —— 3D 穿梭粒子,显示温度湿度时间 IP,闲置后自动激活
5. **配对蓝牙手柄** —— 15 秒快速扫描,自动连接,BOOT 键跳过
6. **贪吃蛇**:方向键操控,A 键开始/重来,吃食物变长,有音效反馈
7. **手柄测试**:所有按键和摇杆的可视化反馈
8. **WiFi 文件服务器** —— 通过网络直传文件到 TF 卡(端口 9999,`tools/send_file.py`)
9. **BOOT 键**长按返回启动器;**SELECT** 瞬间返回

### 📁 目录结构

```
monkeymetal-console/
├── README.md
├── LICENSE                       MIT
├── docs/
│   ├── DEVELOPMENT-GUIDE.md      协作/AI 开发指南
│   ├── gfx-engine.md             图形引擎参考文档
│   └── boot-splash.md            开机动画设计文档
├── main/                         启动序列 + 加载卡带
├── components/
│   ├── gfx_engine/               16bpp 帧缓冲 + Bayer 抖动 + 绘图 API + 8x8 字体
│   ├── lua_runtime/              Lua 5.4 VM + gfx/input/system/audio/sensor binding
│   ├── audio_engine/             ES8311 I²S 编解码驱动 + tone API
│   ├── input_bt_hid/             BLE HID 手柄主机 + 自动重连
│   ├── port_bsp/                 ST7305 LCD(工厂校准 VCOM) + SDMMC 驱动
│   └── wifi_bsp/                 WiFi STA + TCP 文件服务器(端口 9999)
├── tools/
│   └── send_file.py              WiFi 文件传输脚本(PC → 掌机)
├── sdcard_root/
│   ├── games/
│   │   ├── snake/                贪吃蛇卡带
│   │   ├── gamepad_test/         手柄测试卡带
│   │   ├── monkey_leaper/        猴子跳跃平台游戏
│   │   └── space_defender/       太空防御射击游戏
│   └── system/
│       └── launcher/             系统启动器(开机动画 + 屏保 + 设置)
├── partitions.csv                8 MB factory + 4 MB FAT
└── sdkconfig.defaults            PSRAM 八线、BSS 进 PSRAM 等
```

### 🤝 协作方式

我们做**架构 + 产品定义**。具体实现交给遵循
[`docs/DEVELOPMENT-GUIDE.md`](docs/DEVELOPMENT-GUIDE.md)的 AI agent。
人类负责审阅、试玩、提 issue。

**卡带格式和 Lua API 已稳定。** 想写游戏?把 `sdcard_root/games/snake/` 拷一份,改 `cart.lua` 就能跑。

### 📝 许可

MIT。详见 [`LICENSE`](LICENSE)。引擎、示例、工具都是 MIT。
每个游戏卡带在自己目录里带自己的 license。

</details>
