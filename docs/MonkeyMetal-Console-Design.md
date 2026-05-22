# MonkeyMetal Console — 设计与实现规范 v0.1

> **状态:** 设计中 (Design Phase)
> **目标:** 基于 Waveshare ESP32-S3-RLCD-4.2 开发板,打造一台开源、蓝牙手柄控制、可联网下载游戏的 2D 黑白掌机。
> **本文档目的:** 让任意工程师 / 协作 AI 拿到本文后,无需额外背景就能进入实现阶段。

---

## 0. 项目概览

### 0.1 定位

| 维度 | 选择 | 理由 |
|---|---|---|
| 屏幕风格 | 2D 黑白 (1bpp) | 硬件本身是反射屏,阳光下可读,低功耗 |
| 控制方式 | 蓝牙手柄 (BLE HID) | 比有线/物理按键板更通用,支持 8BitDo / Xbox / Switch Pro 等 |
| 游戏来源 | 在线游戏库 + SD 卡安装 | 类似 itch.io 体验,但完全开源 |
| 软件栈 | ESP-IDF + FreeRTOS + 自研游戏引擎 + Lua 脚本游戏 | 引擎用 C 写,游戏用 Lua 写,降低游戏开发门槛 |
| 授权 | MIT (引擎本身),游戏各自独立 License | 无 GPL 传染问题 |

### 0.2 与 gbemu 的关系

`gbemu` 项目走 GameBoy ROM 模拟路线遇到 `rom.bank` 损坏问题。**MonkeyMetal 是一次重新定位**:
- ✅ 复用:LCD 驱动、SD、WiFi、Boot splash、字体绘制、key_test 框架、FreeRTOS 工程骨架
- ❌ 抛弃:gnuboy 核心、gnuboy_sys port、GPL 传染
- ➕ 新增:蓝牙 HID host、Lua VM、游戏引擎 API、在线游戏库协议、Launcher UI

建议在 `gbemu/` 仓库基础上**新建分支** `pivot/monkeymetal`,渐进迁移。

---

## 1. 硬件规格 (Bill of Materials)

### 1.1 主板
| 项 | 规格 |
|---|---|
| 开发板 | Waveshare ESP32-S3-RLCD-4.2 |
| MCU | ESP32-S3 dual-core LX7 @240 MHz |
| RAM | 8 MB Octal PSRAM @80 MHz |
| Flash | 16 MB QIO |
| 屏幕 | ST7305 反射 LCD,400×300,1bpp |
| 卡槽 | TF/microSD,1-bit SDMMC |
| 音频 | ES8311 codec + I2S |
| 无线 | WiFi 2.4G + BT5 (BLE/Classic) |
| 按键 | 板载 PWR / BOOT / RST 三键(只 BOOT 可编程) |

### 1.2 外设建议(选配)
| 项 | 规格 | 用途 |
|---|---|---|
| 蓝牙手柄 | 任意 BLE HID 手柄(8BitDo Lite/SN30 Pro,Xbox Series,Switch Pro) | 主输入设备 |
| 锂电池 + 充电板 | 3.7V 1500–3000 mAh + TP4056 模块 | 便携使用 |
| 喇叭 | 8Ω 1W 微型扬声器 | 接 ES8311 输出 |
| 3D 打印外壳 | 自定义 | 后期社区贡献 |

### 1.3 引脚分配 (固定)
| 功能 | GPIO |
|---|---|
| LCD MOSI/SCK/DC/CS/RST | 12 / 11 / 5 / 40 / 41 |
| SD CLK/CMD/D0 | 38 / 21 / 39 |
| I²C SDA/SCL (ES8311+RTC+SHTC3) | 13 / 14 |
| I²S BCLK/WS/DOUT/MCLK (ES8311) | 待定(参考微雪 07_Audio_Test demo) |
| BOOT 键 | 0 (作系统级菜单键) |

---

## 2. 软件架构

```
┌──────────────────────────────────────────────────┐
│ Apps  (Lua)                                      │
│  ├─ launcher.lua    系统主菜单                    │
│  ├─ store.lua       在线游戏库浏览/下载           │
│  ├─ settings.lua    设置(WiFi/BT/亮度/版本)      │
│  ├─ filemanager.lua 文件管理                     │
│  └─ <game>.lua      第三方游戏(SD 卡里的 .mm)   │
├──────────────────────────────────────────────────┤
│ Lua Runtime (lua 5.4 + 引擎 binding)             │
│  ├─ gfx.*    画图(像素/矩形/精灵/文字/翻页)     │
│  ├─ input.*  按键查询                             │
│  ├─ audio.*  音效播放                             │
│  ├─ storage.* 持久化(per-cart KV)               │
│  ├─ net.*    HTTPS GET / 简单 socket             │
│  └─ system.* 时间/睡眠/重启/退出                 │
├──────────────────────────────────────────────────┤
│ Engine Core (C, 自研)                            │
│  ├─ gfx_engine    16bpp 中间帧 → Bayer 4×4 → 1bpp │
│  ├─ input_router  蓝牙HID + BOOT键 + 按键合并     │
│  ├─ audio_mixer   多通道混音 → I2S → ES8311      │
│  ├─ cart_loader   .mm 解析 + Lua VM 启动         │
│  ├─ store_client  HTTPS 拉 catalog.json          │
│  └─ updater       OTA 自更新                      │
├──────────────────────────────────────────────────┤
│ HAL (基于 ESP-IDF)                                │
│  ├─ st7305_drv    LCD 驱动(从 gbemu 继承)       │
│  ├─ sdmmc_drv     SD 卡 (从 gbemu 继承)          │
│  ├─ bt_hid_host   BLE HID Host                   │
│  ├─ wifi_sta      WiFi (从 gbemu 继承)           │
│  ├─ es8311_drv    音频 codec                     │
│  └─ button_drv    BOOT 键                         │
├──────────────────────────────────────────────────┤
│ ESP-IDF v5.5.x + FreeRTOS                        │
└──────────────────────────────────────────────────┘
```

### 2.1 双核任务划分
| Core | 任务 | 优先级 |
|---|---|---|
| Core 0 | WiFi/BT 协议栈、网络下载、HTTPS、HID 解析 | 系统级 |
| Core 0 | LCD 推屏任务(消费 framebuffer) | 中 |
| Core 0 | 音频混音 + I2S DMA 喂数据 | 中高 |
| Core 1 | Lua VM + 游戏主循环 | 中 |
| Core 1 | 输入路由(BT 事件 → input ringbuffer) | 低 |

设计原则:**游戏逻辑独占 Core 1**,网络 / 音频 / 屏推都在 Core 0,互不干扰。

---

## 3. 显示子系统

### 3.1 帧缓冲管线

```
游戏写入                        引擎处理                       LCD
┌─────────────┐  vsync  ┌──────────────────────┐  SPI3   ┌──────────┐
│ 16bpp FB    │ ──────► │ Bayer 4×4 dither     │ ──────► │ ST7305   │
│ 400×300×2   │         │ → 1bpp 1.5 MB → 15KB │         │ 400×300  │
│ = 240 KB    │         │ 推屏任务(Core 0)    │ 10 MHz  │ 1bpp     │
└─────────────┘         └──────────────────────┘         └──────────┘
   PSRAM                  ~30 ms / 帧                     ~25 fps 上限
```

### 3.2 接口定义 (gfx.* / Lua)

| API | 含义 | 备注 |
|---|---|---|
| `gfx.clear(c)` | 用 0/1/灰阶 0–255 清屏 | 0=黑 255=白 |
| `gfx.pixel(x,y,c)` | 设单像素 | |
| `gfx.line(x1,y1,x2,y2,c)` | Bresenham 直线 | |
| `gfx.rect(x,y,w,h,c,fill)` | 矩形(描边/填充) | |
| `gfx.circle(cx,cy,r,c,fill)` | 圆 | |
| `gfx.sprite(img, x, y)` | 把 image 资源贴到屏 | image = `gfx.load("res/x.mmi")` |
| `gfx.text(font, s, x, y, c)` | 渲染文字 | font 从 cart 加载 |
| `gfx.set_blend(mode)` | NORMAL / XOR / MASK | XOR 用来做选中高亮 |
| `gfx.flip()` | 提交本帧,等下一个 vsync | 阻塞 |

### 3.3 资源格式 (.mmi 图像)
- 头 4B `"MMI1"` + width(u16) + height(u16) + format(u8: 0=1bpp 1=4阶灰度) + data
- 1bpp 行优先,LSB 在左
- 4 阶灰度:每像素 2 bit,渲染时实时 Bayer 抖动到 1bpp

---

## 4. 输入子系统

### 4.1 蓝牙 HID Host(主输入)
- **协议:** BLE HID over GATT(优先)+ BR/EDR HID(后期可选)
- **流程:**
  1. 系统设置里选 **配对模式** → console 进 BLE 主动扫描
  2. 列出附近 HID 设备(Name + RSSI)
  3. 用户选择 → 配对 + 绑定信息存 NVS
  4. 后续启动自动重连绑定的设备

### 4.2 按键映射
**逻辑按键(8 + 2):**

| 逻辑名 | 用途 |
|---|---|
| `UP/DOWN/LEFT/RIGHT` | D-Pad |
| `A` / `B` | 主操作键 |
| `X` / `Y` | 次操作键 |
| `START` | 暂停菜单 |
| `SELECT` | 系统菜单(从游戏退出到 launcher) |

**默认硬件映射(BLE HID 标准 Gamepad usage):**
- 标准 Xbox/Generic Gamepad → 上述按键直接对应
- Switch Pro → 用厂商定制 HID 报告解析(独立处理器)
- 板载 BOOT 键 → 强制系统菜单(避免手柄断连无法退出游戏)

### 4.3 接口定义 (input.* / Lua)

| API | 含义 |
|---|---|
| `input.down(btn)` | 当前帧是否按下,返回 bool |
| `input.pressed(btn)` | 上一帧未按,这一帧按下(脉冲) |
| `input.released(btn)` | 上一帧按,这一帧松开 |
| `input.any()` | 任意键被按下,返回 btn 名或 nil |

`btn` 字符串:`"up"/"down"/"left"/"right"/"a"/"b"/"x"/"y"/"start"/"select"/"system"`。

### 4.4 多手柄支持(后期)
- 协议层最多支持 2 个并发 HID
- player_id 由配对顺序决定
- `input.down(btn, player)` `player` 默认 1

---

## 5. 音频子系统

### 5.1 输出链
- ES8311 8/16/24/48 kHz 立体声 → I2S → 喇叭/耳机
- 引擎默认 16 kHz mono,省 CPU + 省 PSRAM

### 5.2 混音器
- **8 通道软混音**,每通道独立音量、循环、采样率转换
- 通道 0–3: 短音效(枪声/跳跃声),最多 1 秒
- 通道 4–6: 中等音效(语音/角色话)
- 通道 7: BGM 流式播放(从 SD 边读边播)

### 5.3 资源格式 (.mma 音频)
- 头 4B `"MMA1"` + sample_rate(u16) + channels(u8) + samples(u32) + 8-bit unsigned PCM
- 简单粗暴,降编码复杂度

### 5.4 接口定义 (audio.* / Lua)

| API | 含义 |
|---|---|
| `audio.play(snd, volume, loop)` | 播放,返回 voice_id |
| `audio.stop(voice_id)` | 停止单个声音 |
| `audio.tone(freq, duration_ms, volume)` | 生成纯音(无需资源) |
| `audio.bgm(path, volume, loop)` | 流式播放 SD 卡上的 .mma |
| `audio.master(volume)` | 全局音量 0.0–1.0 |

---

## 6. 存储与持久化

### 6.1 SD 卡目录结构

```
/sdcard/
├── system/
│   ├── boot_logo.mmi        启动 logo (替换 MonkeyMetal 默认)
│   ├── theme.lua            launcher 主题
│   └── fw_update.bin        固件 OTA 镜像(临时)
├── games/
│   ├── snake/
│   │   ├── cart.mm          主程序(Lua 字节码或源码)
│   │   ├── meta.json        元信息(名称/作者/版本/图标)
│   │   ├── icon.mmi         32x32 图标
│   │   └── res/             资源(图、声、文本)
│   └── tetris/
│       └── ...
├── save/
│   ├── snake/
│   │   └── kv.bin           per-game 存档(KV 二进制)
│   └── tetris/
│       └── kv.bin
├── screenshots/
│   └── 2026-05-22-snake.mmi
└── log/
    └── crash-<ts>.txt
```

### 6.2 Cart 格式 `.mm`
两种实现路径,选一种:

**方案 A:目录式(推荐 v0.1)**
- 上面 `games/snake/` 整个目录就是一个 cart
- 优点:开发简单,改一行 lua 就能立刻测
- 缺点:文件多,装游戏要拷一坨

**方案 B:打包式(v1.0+)**
- `.mm` 是单文件 zip 容器:`cart.lua` + `meta.json` + `res/*`
- 优点:分发简单(一个文件),Hash 校验方便
- 缺点:需要 zip 解析(miniz 已经在 gbemu 里现成的)

`meta.json` 字段:

| key | 类型 | 说明 |
|---|---|---|
| `name` | string | 游戏名(可中文) |
| `id` | string | 唯一 id,如 `com.monkey.snake` |
| `version` | string | semver `1.2.3` |
| `author` | string | |
| `engine_min` | string | 最低引擎版本,如 `0.1.0` |
| `tags` | string[] | 分类:action/puzzle/rpg... |
| `permissions` | string[] | `net` `audio` `storage` `bt` 任意子集 |
| `entry` | string | 默认 `cart.lua` |

### 6.3 持久化 (storage.* / Lua)

| API | 含义 |
|---|---|
| `storage.set(key, value)` | 立即落 SD,value 任意 Lua 值 |
| `storage.get(key, default)` | 读 |
| `storage.del(key)` | 删 |
| `storage.list()` | 所有 key |

底层是 per-cart 的二进制 KV(MessagePack 编码),写入用日志结构(append-only + 后台 compact)避免磨损。

---

## 7. 在线游戏库

### 7.1 Catalog 协议

**目录服务:** GitHub Pages 或任意静态托管,放一个 `catalog.json`:

```json
{
  "version": 1,
  "updated": "2026-05-22T12:00:00Z",
  "engine_min": "0.1.0",
  "games": [
    {
      "id": "com.monkey.snake",
      "name": "贪吃蛇",
      "author": "MonkeyMetal Team",
      "version": "1.0.0",
      "tags": ["arcade", "classic"],
      "size_bytes": 45120,
      "icon_url": "https://.../snake/icon.mmi",
      "cart_url": "https://.../snake/cart.mm",
      "sha256": "abc123...",
      "screenshot_urls": ["https://.../snake/s1.mmi"],
      "description": "经典贪吃蛇..."
    }
  ]
}
```

### 7.2 Store 应用流程

```
启动 store.lua
  ↓
HTTPS GET catalog.json (HTTPS,带 ETag/If-Modified-Since)
  ↓
显示游戏列表(图标网格 / 文字列表)
  ↓
选游戏 → 详情页(截图、描述、大小)
  ↓
点"安装" →
  - HTTPS GET cart_url 流式下载到 /sdcard/games/<id>/cart.mm
  - 校验 SHA256
  - 解压(如果是打包式)→ /sdcard/games/<id>/
  - 写 install registry: /sdcard/system/installed.json
  ↓
回到 launcher,显示新游戏
```

### 7.3 离线安装
- USB MSC 模式(后期):console 通过 USB 模拟成 U 盘,PC 直接拷贝 `.mm` 到 `games/`
- v0.1 直接物理拔卡 + 读卡器拷贝

### 7.4 安全
- catalog 服务器只允许 HTTPS
- 所有 cart 下载强制 SHA256 校验
- Lua 沙箱:禁止 `os.execute`、`io.open` 全文件路径,只能访问 cart 自己的资源 + storage

---

## 8. Launcher (系统主菜单)

### 8.1 状态机

```
[BOOT_SPLASH]  ← MonkeyMetal 猴子动画 1.5s
     ↓
[CHECK_BT]     ← 重连绑定的手柄 (3s 超时)
     ↓
[LAUNCHER]     ←─────────────────┐
  显示游戏网格 / 列表             │
  ↑↓←→ 选择,A 进入                │
     ↓ A 按下                      │
[GAME_RUN]                        │
  Lua VM 跑游戏                   │
  ↑                              │
  按 SELECT (system) 或 BOOT 键 ──┤
                                 │
[STORE]   ←─ launcher 顶栏选     │
[SETTINGS]←─ launcher 顶栏选     │
[FILES]   ←─ launcher 顶栏选 ────┘
```

### 8.2 视觉设计原则
- **极简**:1bpp 屏幕容不下复杂图形,大量留白
- **大字体**:5×7 太小,主菜单用 8×16 自定义字体
- **2:1 网格**:游戏图标 64×64,4 列 3 行,共 12 个一屏(分页)
- **顶栏 24px**:左侧"游戏""商店""设置""文件",右侧 WiFi/BT 状态 + 时间

---

## 9. 内存预算

### 9.1 DRAM (340 KB,系统已占大头)
| 用途 | 大小 |
|---|---|
| ESP-IDF 内核 + WiFi/BT 栈 | ~250 KB |
| FreeRTOS 任务栈(7 个任务) | ~40 KB |
| 引擎全局 + 输入 ringbuffer | ~20 KB |
| 余量 | ~30 KB |

### 9.2 PSRAM (8 MB)
| 用途 | 大小 |
|---|---|
| 16bpp 主帧缓冲 (400×300×2) | 240 KB |
| 1bpp 推屏缓冲 (双缓冲) | 30 KB |
| Lua VM heap (上限) | 2 MB |
| 游戏资源缓存(纹理/音频) | 3 MB |
| 音频混音缓冲 (8 通道) | 256 KB |
| HTTPS / 网络缓冲 | 256 KB |
| 余量 | ~2 MB |

### 9.3 Flash (16 MB)
| 分区 | 大小 | 用途 |
|---|---|---|
| nvs | 24 KB | WiFi/BT 配置、绑定信息 |
| phy_init | 4 KB | RF 校准 |
| factory | 4 MB | 出厂固件 |
| ota_0 | 4 MB | OTA A 槽 |
| ota_1 | 4 MB | OTA B 槽 |
| storage | 3.9 MB | LittleFS,内置游戏 + 字体 + 默认资源 |

---

## 10. 性能目标

| 指标 | 目标 | 备注 |
|---|---|---|
| 帧率 | 30 FPS | 大多数 2D 游戏够用 |
| 输入延迟(BT 按下到画面变化) | < 60 ms | BT HID 报告 ~15 ms + 引擎 1 帧 33 ms |
| 启动时间(冷) | < 4 s | 从上电到 launcher 可交互 |
| 游戏冷启动 | < 1 s | Lua 加载 + 资源解压 |
| 游戏安装(WiFi) | < 30 s/MB | 取决于网络 |
| 续航(2000 mAh,无背光) | 8–12 小时 | 反射屏没背光巨省电 |

---

## 11. 开发路线图(里程碑)

### M0 — 项目初始化(1 天)
- 新分支 `pivot/monkeymetal`
- 删除 gnuboy/、gnuboy_sys_esp32idf/
- 重命名工程 → `monkeymetal_console`
- README 改成新设计
- 保留 LCD/SD/WiFi/font/key_test 框架

### M1 — 显示引擎核心(3-5 天)
- 16bpp PSRAM framebuffer
- Bayer 4×4 dither 转 1bpp
- 异步推屏任务(Core 0)
- gfx 原语(pixel/line/rect/circle/text/sprite)
- .mmi 图像格式 loader
- **验收:** 绘 demo 画面 + 30 FPS 测速

### M2 — Lua 运行时 + 引擎绑定(5-7 天)
- 集成 Lua 5.4 (esp-idf 有现成 component)
- 实现 gfx.* / input.* / system.* binding
- cart 加载器 (目录式,v0.1)
- 异常 → safe panic screen
- **验收:** 跑一个 Lua hello world 在屏上

### M3 — 内置贪吃蛇 demo(2-3 天)
- `games/snake/` cart
- 完整一局玩法
- 用 BOOT 键当作临时输入(蓝牙还没好)
- **验收:** 贪吃蛇能从 launcher 启动 + 玩到 Game Over + 存最高分

### M4 — 音频(3 天)
- ES8311 I2S 驱动
- 8 通道混音器
- audio.* binding
- .mma 格式 loader
- **验收:** 贪吃蛇加吃食物音效 + BGM

### M5 — 蓝牙手柄(7-10 天)⚠️ 最难一段
- BLE HID Host stack 集成 (ESP-IDF 5.x BLE Host bluedroid)
- 配对 UI(扫描 + 列表 + 选择)
- 标准 Gamepad usage 解析
- 绑定信息存 NVS,断电不丢
- BOOT 键作为应急输入
- **验收:** 用 8BitDo SN30 Pro 玩贪吃蛇

### M6 — Launcher(3-5 天)
- 4×3 游戏图标网格
- 顶栏导航(游戏/商店/设置/文件)
- 系统菜单(暂停 + 退出游戏)
- 设置应用(WiFi/BT/亮度/版本)
- **验收:** 完整可用的系统首页,无 game 也能用

### M7 — 在线商店(5-7 天)
- HTTPS client (esp-tls)
- catalog.json 解析(cJSON)
- 游戏列表 / 详情 / 安装流程
- SHA256 校验
- 已装游戏管理
- **验收:** 从空 SD 卡拉一个新游戏完成安装并启动

### M8 — OTA 自更新(3 天)
- 引擎版本号 + catalog 字段对比
- 通过 HTTPS 拉新固件
- esp_https_ota
- **验收:** 不用拔卡升级引擎

### M9+ — 长尾优化
- 多手柄(2P 联机,本地)
- 蓝牙 Classic A2DP 输出(蓝牙耳机)
- USB MSC 模式
- 文件管理 / 截图浏览器
- 主题系统
- 字体国际化(中文)

---

## 12. 仓库结构 (M1 完成后)

```
monkeymetal_console/
├── README.md / README_zh.md
├── LICENSE                        MIT
├── CHANGELOG.md
├── docs/
│   ├── MonkeyMetal-Console-Design.md  本文档
│   ├── api/
│   │   ├── gfx.md                 完整 API 参考
│   │   ├── input.md
│   │   └── audio.md
│   ├── cart-format.md             怎么打包游戏
│   ├── store-protocol.md          catalog.json 协议
│   └── hardware/
│       ├── pinout.md
│       └── enclosure.md           外壳设计(后期)
├── firmware/
│   ├── CMakeLists.txt
│   ├── partitions.csv
│   ├── sdkconfig.defaults
│   ├── main/
│   │   └── main.cpp               app_main 启动 launcher
│   ├── components/
│   │   ├── hal_st7305/            LCD HAL(从 gbemu 继承)
│   │   ├── hal_sdmmc/             SD HAL
│   │   ├── hal_es8311/            音频 HAL
│   │   ├── hal_bt_hid/            蓝牙 HID host
│   │   ├── hal_wifi/              WiFi HAL
│   │   ├── engine_gfx/            画图核心 + dither + 字体
│   │   ├── engine_input/          输入路由
│   │   ├── engine_audio/          混音器
│   │   ├── engine_storage/        KV 存档
│   │   ├── engine_net/            HTTPS client
│   │   ├── lua_runtime/           Lua 5.4 + binding
│   │   ├── cart_loader/           .mm 加载
│   │   ├── store_client/          在线商店逻辑
│   │   └── updater/               OTA
│   └── apps/
│       ├── launcher/              系统首页(Lua)
│       ├── store/                 在线商店(Lua)
│       ├── settings/              设置(Lua)
│       └── filemanager/           文件管理(Lua)
├── carts/                         示例游戏(开发者参考)
│   ├── snake/
│   │   ├── cart.lua
│   │   ├── meta.json
│   │   ├── icon.mmi
│   │   └── res/
│   │       ├── eat.mma
│   │       └── gameover.mma
│   ├── tetris/
│   └── pong/
├── tools/
│   ├── mmi_converter.py           PNG → .mmi 转换
│   ├── mma_converter.py           WAV → .mma 转换
│   ├── cart_packer.py             目录 → .mm zip
│   ├── catalog_builder.py         扫描 carts/ 生成 catalog.json
│   └── usb_pad_emu.py             开发期键盘 → BLE HID 桥(可选)
├── catalog/                       发布到 GitHub Pages 的目录
│   ├── catalog.json               游戏库索引
│   └── games/
│       └── <id>/
│           ├── cart.mm
│           ├── icon.mmi
│           └── screenshots/
└── hardware/                      硬件设计(后期)
    ├── enclosure-cad/
    ├── button-board-kicad/        外接物理按键板(可选)
    └── battery-mod/
```

---

## 13. Lua 游戏开发示例(贪吃蛇)

伪代码,展示引擎 API 的使用方式,实际 cart.lua 就长这样:

```lua
-- snake/cart.lua

local W, H = 400, 300
local CELL = 10
local COLS, ROWS = W // CELL, H // CELL  -- 40 x 30 格

local snake, dir, food, score, dead

local function reset()
    snake = { {x=20, y=15} }
    dir = "right"
    food = {x=10, y=10}
    score = 0
    dead = false
end

local function on_input()
    if input.pressed("up") and dir ~= "down" then dir = "up" end
    if input.pressed("down") and dir ~= "up" then dir = "down" end
    if input.pressed("left") and dir ~= "right" then dir = "left" end
    if input.pressed("right") and dir ~= "left" then dir = "right" end
    if dead and input.pressed("a") then reset() end
end

local function step()
    if dead then return end
    local head = snake[1]
    local nx, ny = head.x, head.y
    if dir == "up" then ny = ny - 1
    elseif dir == "down" then ny = ny + 1
    elseif dir == "left" then nx = nx - 1
    elseif dir == "right" then nx = nx + 1 end

    -- 撞墙或自咬
    if nx < 0 or nx >= COLS or ny < 0 or ny >= ROWS then dead = true; return end
    for _, s in ipairs(snake) do
        if s.x == nx and s.y == ny then dead = true; return end
    end

    table.insert(snake, 1, {x=nx, y=ny})
    if nx == food.x and ny == food.y then
        score = score + 1
        audio.tone(880, 50, 0.5)
        food.x = math.random(0, COLS-1)
        food.y = math.random(0, ROWS-1)
    else
        table.remove(snake)
    end
end

local function draw()
    gfx.clear(255)  -- 白底
    -- 蛇
    for _, s in ipairs(snake) do
        gfx.rect(s.x*CELL, s.y*CELL, CELL-1, CELL-1, 0, true)
    end
    -- 食物
    gfx.rect(food.x*CELL, food.y*CELL, CELL-1, CELL-1, 0, false)
    -- 分数
    gfx.text(font, "SCORE " .. score, 4, 4, 0)
    if dead then
        gfx.text(font, "GAME OVER - PRESS A", 80, 140, 0)
    end
    gfx.flip()
end

-- 入口
reset()
local font = gfx.load_font("system:default8x16")
local frame = 0
while not system.should_exit() do
    on_input()
    if frame % 6 == 0 then step() end  -- 每 6 帧动一格 = 5 step/秒
    draw()
    frame = frame + 1
end

-- 退出前存最高分
local best = storage.get("best", 0)
if score > best then storage.set("best", score) end
```

---

## 14. 开放性问题(待团队决议)

| 问题 | 待决 |
|---|---|
| Lua VM 是用 Lua 5.4 还是 LuaJIT? | 5.4(LuaJIT 的 ARM/Xtensa 支持差) |
| catalog 服务器是 GitHub Pages 还是自建? | v0.1 GitHub Pages,长期看流量 |
| 游戏内 IAP / 付费? | 不做,纯免费开源 |
| 中文输入法? | 不做,文字内容由游戏自己嵌入 |
| 蓝牙 vs USB OTG 手柄? | 优先 BLE,USB 后期可选 |
| 多人游戏怎么联机? | v1.0 本地多手柄,v2.0 WiFi UDP |
| 是否兼容跑 GameBoy ROM? | 移到独立 fork (`monkey-emu` 项目),不混进主线 |

---

## 15. 验收清单(v0.1 发布要求)

发布 v0.1 之前必须满足:

- [ ] M1–M3 全部完成,贪吃蛇可玩
- [ ] M4 音频出声,至少有 1 个音效
- [ ] M5 至少 1 款蓝牙手柄(8BitDo SN30 Pro 作 reference)能配对游戏
- [ ] M6 launcher 完整,无手柄也能用 BOOT 键导航
- [ ] README 中英双语
- [ ] LICENSE = MIT
- [ ] 一份 5 分钟视频 demo(从开机到玩游戏)
- [ ] 至少 3 款示例游戏 (Snake, Tetris, Pong)
- [ ] catalog.json 站点上线,Store 能拉到 1 款新游戏

---

## 16. 致谢

- **gbemu** 项目积累的硬件初始化、字体、Boot splash 框架
- **Waveshare** 提供的 ST7305/SDMMC/ES8311 demo 代码
- **Espressif** ESP-IDF
- **Lua** 团队
- 未来贡献者

---

**文档版本:** v0.1
**最后更新:** 2026-05-22
**编写:** MonkeyMetal Team
