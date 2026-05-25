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

## 这是什么

**MonkeyMetal Console** 是一台基于
[微雪 ESP32-S3-RLCD-4.2](https://www.waveshare.net/wiki/ESP32-S3-RLCD-4.2)
开发板的开源掌机 —— 一台可折腾、可在阳光下阅读的 2D 黑白迷你主机：

- **反射式 1bpp LCD**（400×300）—— 阳光下可读、功耗极低
- **蓝牙手柄**（8BitDo / Xbox / Switch Pro 都能用）
- **Lua 脚本游戏运行时** —— 会写 Lua 就能做游戏
- **ES8311 音频** —— I²S 编解码器，支持音效
- **SHTC3 温湿度传感器** —— 真实环境数据采集
- **PCF85063 RTC + SNTP 网络校时** —— WiFi 连接自动同步精确时间
- **WiFi 文件服务器** —— 网络直传文件到 TF 卡，免拔插
- **星空屏保** —— 3D 穿梭粒子特效 + 系统信息展示
- **MIT 协议** —— 随便 fork、改、商用

它**不是模拟器**，跑的是专门为这台机器写的游戏。

## 硬件规格

| 项目 | 规格 |
| --- | --- |
| 开发板 | [微雪 ESP32-S3-RLCD-4.2](https://www.waveshare.net/wiki/ESP32-S3-RLCD-4.2) |
| 主控 | ESP32-S3 双核 Xtensa LX7 @240 MHz |
| RAM | 8 MB 八线 PSRAM @80 MHz |
| Flash | 16 MB QIO |
| 显示 | ST7305 反射屏，400×300，1bpp |
| 传感器 | SHTC3（温湿度）+ PCF85063（RTC 时钟） |
| 存储 | TF/microSD，1-bit SDMMC |
| 音频 | ES8311 编解码 + I²S |
| 无线 | WiFi 2.4G + 蓝牙 5（BLE） |
| 供电 | USB-C，选配 3.7V 锂电 + TP4056 |

## v1.0.1 功能

| 功能 | 状态 |
| --- | --- |
| 图形引擎（16bpp 帧缓冲 + Bayer 4×4 抖动 + 绘图 API + 8×8 字体） | ✅ |
| Lua 5.4 运行时 + 卡带加载器（沙箱 VM，PSRAM 分配器） | ✅ |
| 蓝牙 HID 手柄（8BitDo / Xbox / Switch Pro，自动重连，快速配对） | ✅ |
| 音频引擎（ES8311 I²S 编解码，tone/beep API） | ✅ |
| 系统启动器（浏览游戏、设置页、开机动画、星空屏保） | ✅ |
| **星空屏保**（3D 穿梭粒子，显示温湿度/时间/IP，闲置自动激活） | ✅ v1.0.1 |
| **SHTC3 温湿度传感器**（I2C 读取，已校准 -4°C） | ✅ v1.0.1 |
| **PCF85063 RTC + SNTP 网络校时**（WiFi 连接自动同步） | ✅ v1.0.1 |
| **WiFi 文件服务器**（端口 9999，`tools/send_file.py` 网络传文件） | ✅ v1.0.1 |
| **设置页面**（音量、屏保超时 5-60 秒、WiFi 状态、IP 显示） | ✅ v1.0.1 |
| **开机动画**（超光速星空 + 系统自检列表 + 扫描线 Logo） | ✅ v1.0.1 |
| **像素大字渲染**（块状大号数字，时间与数值清晰可读） | ✅ v1.0.1 |
| 贪吃蛇游戏（方向键操控，吃/死音效，分数记录） | ✅ |
| 手柄测试卡带（按键/摇杆可视化测试） | ✅ |
| BOOT 键备用（GPIO 0，长按返回启动器） | ✅ |

## 内置游戏

### 🍌 猴子跳跃（Monkey Leaper）
横版平台跳跃游戏。控制小猴子左右移动、跳跃躲避尖刺。收集香蕉得分，到达终点旗帜过关。
- **操作**：方向键（移动），上/A（跳跃），START（开始/重开），SELECT（退出）
- **特色**：多关卡、物理跳跃手感、复古 1bpp 像素美术

### 🚀 太空防御（Space Defender）
经典太空射击游戏。驾驶飞船左右移动，A/B 键发射子弹消灭敌机。不同敌机分值不同。每 1000 分出现 Boss，击败获得额外奖励。
- **操作**：方向键（移动），A/B（射击），START（开始/重开），SELECT（退出）
- **特色**：完整音频系统（背景音乐 + 射击/受击/爆炸音效）、Boss 战、分数排行

### 🐍 贪吃蛇（Snake）
经典贪吃蛇。吃食物变长，碰墙或碰自己结束。分数越高速度越快，记录最高分。
- **操作**：方向键（转向），A（开始/重开）
- **特色**：实时分数条、最高分记录、速度递增

### 🎮 手柄测试（Gamepad Test）
诊断工具。所有按键、方向键、摇杆的可视化反馈。实时显示连接状态。
- **操作**：按任意键点亮对应区域
- **特色**：摇杆数值显示、按键状态实时刷新

## 快速上手

### 准备
- ESP-IDF v5.5.x + VS Code（安装 Espressif IDF 扩展）
- 微雪 ESP32-S3-RLCD-4.2 + Type-C 线
- FAT32 格式化的 TF 卡
- 蓝牙手柄（8BitDo / Xbox / Switch Pro）

### 编译烧录
```bash
git clone https://github.com/MonkeyMetal/monkeymetal-console.git
cd monkeymetal-console
cp components/wifi_bsp/user_secrets.h.example components/wifi_bsp/user_secrets.h
# 编辑 user_secrets.h 填写 WiFi 凭据

idf.py set-target esp32s3
idf.py -p COMx build flash monitor
```

VS Code 用户：`Ctrl+Shift+P` → `ESP-IDF: Build, Flash and Monitor`

### 拷贝游戏到 TF 卡
把 `sdcard_root/` 目录下所有内容复制到 FAT32 TF 卡根目录。

### WiFi 文件传输（免拔卡）
烧录后 WiFi 连接成功，从串口日志找到 IP（`GOT IP: x.x.x.x`）：
```bash
# 发送单个文件
python tools/send_file.py 192.168.x.x 本地文件.lua /games/我的游戏/cart.lua

# 发送整个目录
python tools/send_file.py 192.168.x.x --dir sdcard_root/system/launcher /system/launcher
```

### 启动流程
1. **开机动画** —— 超光速星空穿梭 + 系统自检列表 + 复古扫描线 Logo
2. **系统启动器** —— 浏览并启动 TF 卡上的游戏
3. **设置页面**（SELECT 键）—— 音量、屏保超时（5-60 秒）、WiFi 状态、IP 地址
4. **星空屏保** —— 闲置超时后自动激活，显示时间/温湿度/IP
5. **配对蓝牙手柄** —— 15 秒快速扫描，自动连接，BOOT 键跳过
6. **BOOT 键**长按返回启动器；**SELECT** 瞬间返回

## 目录结构

```
monkeymetal-console/
├── README.md / README_CN.md     项目文档（中/英）
├── LICENSE                       MIT
├── docs/
│   ├── DEVELOPMENT-GUIDE.md      协作/AI 开发指南
│   ├── gfx-engine.md             图形引擎参考文档
│   └── boot-splash.md            开机动画设计文档
├── main/                         启动序列 + 加载卡带
├── components/
│   ├── gfx_engine/               16bpp 帧缓冲 + Bayer 抖动 + 绘图 API + 8×8 字体
│   ├── lua_runtime/              Lua 5.4 VM + gfx/input/system/audio/sensor binding
│   ├── audio_engine/             ES8311 I²S 编解码驱动 + tone API
│   ├── input_bt_hid/             BLE HID 手柄主机 + 自动重连
│   ├── port_bsp/                 ST7305 LCD（工厂校准 VCOM）+ SDMMC 驱动
│   └── wifi_bsp/                 WiFi STA + TCP 文件服务器（端口 9999）
├── tools/
│   └── send_file.py              WiFi 文件传输脚本（PC → 掌机）
├── sdcard_root/
│   ├── games/
│   │   ├── snake/                贪吃蛇
│   │   ├── gamepad_test/         手柄测试
│   │   ├── monkey_leaper/        猴子跳跃（平台游戏）
│   │   └── space_defender/       太空防御（射击游戏）
│   └── system/
│       └── launcher/             系统启动器（开机动画 + 屏保 + 设置）
├── partitions.csv                factory 8 MB + FAT 4 MB
└── sdkconfig.defaults            PSRAM 八线、BSS 进 PSRAM 等
```

## 协作方式

我们做**架构 + 产品定义**。具体实现交给遵循
[`docs/DEVELOPMENT-GUIDE.md`](docs/DEVELOPMENT-GUIDE.md) 的 AI agent。
人类负责审阅、试玩、提 issue。

卡带格式和 Lua API 已稳定。把 `sdcard_root/games/snake/` 拷一份，改 `cart.lua` 就能做自己的游戏。

## 更新日志

### v1.0.1 (2026-05-26)
- 🆕 星空屏保（3D 穿梭粒子 + 系统信息面板 + 猴子吉祥物）
- 🆕 SHTC3 真实温湿度传感器读取（I2C，已校准 -4°C）
- 🆕 PCF85063 RTC + SNTP 网络校时
- 🆕 WiFi 文件服务器（端口 9999）
- 🆕 开机动画（超光速星空 + 系统自检 + 扫描线）
- 🆕 设置页面（音量、屏保超时、WiFi/IP 显示）
- 🆕 像素大字渲染（时间用大块状数字）
- 🔧 修复 input.pressed() 边沿检测（latched flag）
- 🔧 修复显示屏对比度（VCOM 工厂校准）
- 🔧 修复启动器按键导航（改用 input.pressed）
- 🔧 蓝牙配对界面优化（15 秒超时 + BOOT 跳过）

### v1.0.0 (2026-05-24)
- 首个正式版本
- 图形引擎 + Lua 运行时
- 蓝牙手柄 + 音频引擎
- 贪吃蛇 + 手柄测试 + 启动器

## 许可

MIT。详见 [`LICENSE`](LICENSE)。引擎、示例、工具都是 MIT。
每个游戏卡带在自己目录里带自己的 license。
