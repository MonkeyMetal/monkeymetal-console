<div align="center">

# 🐒 MonkeyMetal Console

**一台开源 2D 黑白掌机 / An open-source 2D black-and-white handheld console**

[📖 中文文档](README_CN.md) · [English](README.md)

![version](https://img.shields.io/badge/version-v1.0.1-brightgreen)
![license](https://img.shields.io/badge/license-MIT-blue)
![board](https://img.shields.io/badge/board-ESP32--S3--RLCD--4.2-red)
![idf](https://img.shields.io/badge/ESP--IDF-v5.5.x-green)
![input](https://img.shields.io/badge/input-Bluetooth%20HID-blueviolet)
![audio](https://img.shields.io/badge/audio-ES8311%20I²S-orange)

</div>

---

## What is this?

**MonkeyMetal Console** is an open-source handheld game console built on the
[Waveshare ESP32-S3-RLCD-4.2](https://www.waveshare.net/wiki/ESP32-S3-RLCD-4.2)
dev board — a hackable, sunlight-readable 2D black-and-white mini console with:

- **Reflective 1bpp LCD** (400×300) — readable outdoors, ultra-low power
- **Bluetooth HID controllers** (8BitDo / Xbox / Switch Pro)
- **Lua-scripted game runtime** — anyone who knows Lua can ship a game
- **ES8311 audio** — I²S codec with tone/sfx support
- **SHTC3 temperature + humidity sensor** — real-time environment monitoring
- **PCF85063 RTC + SNTP time sync** — accurate clock via WiFi
- **WiFi file server** — send files to TF card over the network
- **Starfield screensaver** — 3D warp particles with system info display
- **MIT licensed engine** — fork it, mod it, sell it

This is **not an emulator**. It runs games made specifically for this console.

## Hardware

| Item | Spec |
| --- | --- |
| Board | [Waveshare ESP32-S3-RLCD-4.2](https://www.waveshare.net/wiki/ESP32-S3-RLCD-4.2) |
| MCU | ESP32-S3 dual-core Xtensa LX7 @240 MHz |
| RAM | 8 MB Octal PSRAM @80 MHz |
| Flash | 16 MB QIO |
| Display | ST7305 reflective LCD, 400×300, 1bpp |
| Sensors | SHTC3 (temp/humidity) + PCF85063 (RTC) |
| Storage | TF/microSD, 1-bit SDMMC |
| Audio | ES8311 codec + I²S |
| Wireless | Wi-Fi 2.4 G + Bluetooth 5 (BLE) |
| Power | USB-C, optional 3.7 V Li-ion + TP4056 |

## v1.0.1 Features

| Feature | Status |
| --- | --- |
| Graphics engine (16bpp framebuffer + Bayer 4×4 dither + draw API + 8×8 font) | ✅ |
| Lua 5.4 runtime + cart loader (sandboxed VM, PSRAM allocator) | ✅ |
| Bluetooth HID gamepad (8BitDo / Xbox / Switch Pro, auto-reconnect, fast pairing) | ✅ |
| Audio engine (ES8311 I²S codec, tone/beep API) | ✅ |
| System launcher (browse, settings, boot animation, starfield screensaver) | ✅ |
| **Starfield screensaver** (3D warp particles, temp/humidity/time/IP, auto-activate) | ✅ v1.0.1 |
| **SHTC3 temperature + humidity** (I2C sensor, calibrated -4°C) | ✅ v1.0.1 |
| **PCF85063 RTC + SNTP time sync** (network time via WiFi) | ✅ v1.0.1 |
| **WiFi file server** (port 9999, `tools/send_file.py` for network file transfer) | ✅ v1.0.1 |
| **Settings page** (volume, screensaver timeout 5-60s, WiFi status, IP) | ✅ v1.0.1 |
| **Boot animation** (hyper-drive starfield + system checklist + scanline logo) | ✅ v1.0.1 |
| **Big digit pixel font** (block-style large numbers for time and values) | ✅ v1.0.1 |
| Snake game (D-Pad + A button, eat/die sounds, score tracking) | ✅ |
| Gamepad test cart (visual button/joystick tester) | ✅ |
| BOOT key fallback (GPIO 0, long-press back to launcher) | ✅ |

## Built-in Games

### 🍌 Monkey Leaper
A side-scrolling platformer. Control the monkey with D-Pad to run left/right and jump over obstacles. Collect bananas for points, avoid deadly spikes, and reach the goal flag at the end of each level.
- **Controls**: D-Pad (move), Up/A (jump), START (start/restart), SELECT (quit)

### 🚀 Space Defender
A classic space shooter. Pilot your ship left/right, blast enemy waves with A/B buttons. Different enemies award different points. A boss appears every 1000 points — defeat it for bonus score.
- **Controls**: D-Pad (move), A/B (shoot), START (start/restart), SELECT (quit)
- **Audio**: Full sound system with background music, shoot/hit/explosion SFX

### 🐍 Snake
The classic. Eat food to grow, avoid walls and yourself. Speeds up as your score increases. High score tracking.
- **Controls**: D-Pad (steer), A (start/restart)

### 🎮 Gamepad Test
Diagnostic tool. Visual feedback for all buttons, D-Pad, and analog sticks. Shows real-time connection status.
- **Controls**: Press any button to see it light up

## Quick Start

### Prerequisites
- ESP-IDF v5.5.x + VS Code with Espressif IDF extension
- Waveshare ESP32-S3-RLCD-4.2 + USB-C cable
- FAT32-formatted TF card
- Bluetooth gamepad (8BitDo / Xbox / Switch Pro)

### Build & Flash
```bash
git clone https://github.com/MonkeyMetal/monkeymetal-console.git
cd monkeymetal-console
cp components/wifi_bsp/user_secrets.h.example components/wifi_bsp/user_secrets.h
# edit user_secrets.h with your Wi-Fi SSID/password

idf.py set-target esp32s3
idf.py -p COMx build flash monitor
```

### Copy games to TF card
Copy the entire `sdcard_root/` contents to the root of your FAT32 TF card.

### WiFi file transfer (no card removal)
After flashing and WiFi connects, find the IP from serial log (`GOT IP: x.x.x.x`):
```bash
# Send a single file
python tools/send_file.py 192.168.x.x local_file.lua /games/my_game/cart.lua

# Send a directory
python tools/send_file.py 192.168.x.x --dir sdcard_root/system/launcher /system/launcher
```

### What you'll see
1. **Boot animation** — hyper-drive starfield + system checklist + retro scanline logo
2. **Launcher** — browse & launch games from TF card
3. **Settings** (SELECT key) — volume, screensaver timeout (5-60s), WiFi status, IP
4. **Starfield screensaver** — activates after idle timeout, shows time/temp/humidity/IP
5. **Pair gamepad** — quick 15s scan, auto-connect, BOOT key to skip
6. **BOOT key** long-press → back to launcher; **SELECT** → instant return

## Layout

```
monkeymetal-console/
├── README.md / README_CN.md
├── LICENSE                       MIT
├── docs/
│   ├── DEVELOPMENT-GUIDE.md      contribution & AI agent rules
│   ├── gfx-engine.md             graphics engine reference
│   └── boot-splash.md            boot animation design
├── main/                         boot sequence + cart launch
├── components/
│   ├── gfx_engine/               16bpp framebuffer + Bayer dither + draw API + 8×8 font
│   ├── lua_runtime/              Lua 5.4 VM + gfx/input/system/audio/sensor bindings
│   ├── audio_engine/             ES8311 I²S codec driver + tone API
│   ├── input_bt_hid/             BLE HID gamepad host + auto-reconnect
│   ├── port_bsp/                 ST7305 LCD (factory-calibrated VCOM) + SDMMC
│   └── wifi_bsp/                 Wi-Fi STA + TCP file server (port 9999)
├── tools/
│   └── send_file.py              WiFi file transfer (PC → console)
├── sdcard_root/
│   ├── games/
│   │   ├── snake/                Snake
│   │   ├── gamepad_test/         Gamepad tester
│   │   ├── monkey_leaper/        Monkey Leaper platformer
│   │   └── space_defender/       Space Defender shooter
│   └── system/
│       └── launcher/             Launcher (boot anim + screensaver + settings)
├── partitions.csv                factory 8 MB + FAT 4 MB
└── sdkconfig.defaults            PSRAM Octal, BSS-in-PSRAM, etc.
```

## Contributing

Architecture & product decisions are human-led. Implementation is delegated to AI agents following
[`docs/DEVELOPMENT-GUIDE.md`](docs/DEVELOPMENT-GUIDE.md).
Humans handle review, playtesting, and shipping.

The cart format and Lua API are stable. Copy `sdcard_root/games/snake/` as a template to start your own game.

## Changelog

### v1.0.1 (2026-05-26)
- 🆕 Starfield screensaver (3D warp particles + system info panel + monkey mascot)
- 🆕 SHTC3 real temp/humidity sensor (I2C, calibrated -4°C)
- 🆕 PCF85063 RTC + SNTP network time sync
- 🆕 WiFi file server (port 9999)
- 🆕 Boot animation (hyper-drive starfield + system checklist + scanline)
- 🆕 Settings page (volume, screensaver timeout, WiFi/IP display)
- 🆕 Big digit pixel font rendering for time display
- 🔧 Fixed input.pressed() edge detection (latched flag)
- 🔧 Fixed display contrast (VCOM factory calibration)
- 🔧 Fixed launcher navigation (switched to input.pressed)
- 🔧 Bluetooth pairing timeout optimised (15s, BOOT to skip)

### v1.0.0 (2026-05-24)
- First stable release
- Graphics engine + Lua runtime + BLE gamepad + audio
- Snake, Gamepad Test, and System Launcher

## License

MIT. See [`LICENSE`](LICENSE). Engine, examples, and tools are all MIT.
Each game cart ships with its own license.
