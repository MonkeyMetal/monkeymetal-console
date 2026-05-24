# MonkeyMetal Console — 开发指南 (Development Guide)

> **本文档读者:** 接手实现工作的协作 AI / 工程师。
> **本文档约束力:** 强约束。不遵守这里的规则的 PR 会被打回。
> **配套文档:** [`README.md`](../README.md)(项目入口) + [`gfx-engine.md`](gfx-engine.md)(图形引擎)。

---

## 0. 上手第一步

每次进入新会话,**先读完下面两份文档,再动手**:

1. `README.md` —— 当前项目状态、v1.0.0 功能、编译路径。
2. 本文件 —— 工作流、代码规范、提交规范。

读完后,运行一次 `idf.py build`,确认你的本地环境可用。**未跑过 build 不要写新代码**。

---

## 1. 工程结构与边界

```
monkeymetal-console/
├── main/                      应用入口与启动流程,只放 app_main / 启动序列
├── components/
│   ├── port_bsp/              所有硬件相关代码 (LCD / SD / 按键 / 电源)
│   ├── wifi_bsp/              Wi-Fi STA + 网络栈
│   ├── gfx_engine/            16bpp 帧缓冲、Bayer 4×4 抖动、绘图 API、文字渲染
│   ├── lua_runtime/           Lua 5.4 VM + gfx/input/system/audio binding
│   ├── audio_engine/          ES8311 I²S 编解码驱动 + tone/beep API
│   └── input_bt_hid/          BLE HID 手柄主机 + 按键映射 + 自动重连
├── docs/                      架构与开发文档
├── partitions.csv             分区表 (factory 8 MB + FAT 4 MB)
└── sdkconfig.defaults         默认配置(PSRAM Octal、BSS in PSRAM 等)
```

### 1.1 组件边界规则

- **`main/` 只放启动逻辑**。任何硬件访问、游戏逻辑、网络访问都必须封装到组件里。
- **组件之间通过头文件公开的 API 交互**,不允许跨组件 `#include "../xxx/private.h"`。
- 每个组件的 `CMakeLists.txt` 只 `REQUIRES` 它真正用到的组件,不要写一个全集。
- **新增组件:** 必须先在文档里登记其职责,再开 PR。临时实验代码留在分支,不进 main。

### 1.2 严禁事项

- ❌ 引入 GPL/AGPL 依赖(本项目 MIT,做不到二次分发)。
- ❌ 拷贝 gnuboy / 任何 GameBoy 模拟器代码进来 —— 项目已脱离模拟器路线。
- ❌ 在 `main/` 里直接读 GPIO / 直接发 SPI。一律走 `port_bsp`。
- ❌ 把 Wi-Fi 凭据写死在源码里。一律读 `components/wifi_bsp/user_secrets.h`(已 `.gitignore`)。

---

## 2. 编译 / 烧录 / 调试

### 2.1 命令行

```bash
# 1. 设置目标(只需一次)
idf.py set-target esp32s3

# 2. 改 sdkconfig(可选)
idf.py menuconfig

# 3. 编译
idf.py build

# 4. 烧录 + 串口
idf.py -p COMx flash monitor
```

退出 monitor: `Ctrl+]`。

### 2.2 VS Code

`Ctrl+Shift+P` → `ESP-IDF: Build, Flash and Monitor your Project`。
USB-JTAG 直接抓 log,不用外接 USB-TTL。

### 2.3 PSRAM / 链接配置

`sdkconfig.defaults` 已经设好:
- `CONFIG_SPIRAM_MODE_OCT=y` —— Octal PSRAM @80 MHz
- `CONFIG_SPIRAM_USE_MALLOC=y` —— `malloc` 默认走 PSRAM
- `CONFIG_SPIRAM_ALLOW_BSS_SEG_EXTERNAL_MEMORY=y` —— 大数组用 `EXT_RAM_BSS_ATTR`

**改这些之前先在文档里说明理由**。

### 2.4 帧缓冲 / 大数组

```c
// 4 KB 以下:正常 .bss / 栈 都行
uint8_t small_buf[1024];

// 4 KB 以上:放 PSRAM
EXT_RAM_BSS_ATTR static uint8_t framebuffer[400 * 300 / 8];

// 运行期分配:malloc 默认走 PSRAM,但 DMA 需要内部 RAM:
void *dma_buf = heap_caps_malloc(size, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
```

---

## 3. 代码规范

### 3.1 1bpp 显示设计规则（必读）

ST7305 是 400×300 反射式 **1bpp 黑白屏**，通过 Bayer 4×4 有序抖动把 16bpp 灰阶转成 1bpp。**抖动算法会让中等灰度的细线变成虚线/点阵**——这是硬件限制,不是 bug。

**铁律（游戏开发者必须遵守）：**

| 用途 | 推荐值 | 为什么 |
|---|---|---|
| **文字**（激活态） | `GFX_BLACK (0)` 或 `GFX_WHITE (255)` | 纯色不经过抖动,笔画清晰 |
| **文字**（置灰/未激活） | `GFX_GRAY(32～64)` | 深灰在抖动后仍有 12-25% 密度,能辨认 |
| **细线 / 描边** | **只用纯黑(0)**,线宽 ≥2px 更稳 | 中灰细线 → 抖动成虚线,完全不可读 |
| **填充 / 实心形状** | `GFX_BLACK (0)` 或 `GFX_GRAY(192+)` | 大面积填充,抖动能伪造灰阶 |
| **❌ 禁止** | `GFX_GRAY(100～180)` 用于细线或文字 | 抖动后变成稀疏点阵,什么也看不清 |

**验证方法：** 在真机屏幕上确认,不要凭 PC 模拟器判断。反射屏对比度低,PC 上看是灰的在真机上可能就是空白。

### 3.2 C 代码

- **风格:** ESP-IDF / Linux kernel 风,4 空格缩进,大括号同行。
- **命名:** `snake_case` 函数与变量,`UPPER_SNAKE` 宏,文件名小写。
- **公共 API 前缀:** 组件名前缀,如 `gfx_draw_pixel(...)`、`port_bsp_lcd_flush(...)`。
- **错误返回:** `esp_err_t` + `ESP_ERROR_CHECK` 用于不可恢复错误,正常路径返回错误码。
- **日志:** `ESP_LOGI(TAG, ...)`,每个 `.c` 文件顶部 `static const char *TAG = "module";`。
- **C++ :** 仅在必须时使用,文件后缀 `.cpp`。绝大多数文件是 `.c`。

### 3.3 Lua 代码

- 文件名 `snake_case.lua`。
- 每个卡带必须导出 `init() / update() / draw()`。
- Lua 5.4 标准库,**不用 LuaJIT**。
- Lua 层颜色参数约定 0–255 灰阶,binding 层统一转 `GFX_GRAY(c)`。

### 3.4 注释

- 注释解释**为什么**,不解释**做什么**(代码本身说明做什么)。
- 已读懂的旧代码不要补注释。
- 公共 API 必须有 doxygen 风简短说明:
  ```c
  /// @brief 把 1bpp 帧缓冲推到 ST7305。阻塞直到 DMA 完成。
  esp_err_t gfx_flush(const uint8_t *fb);
  ```

---

## 4. 工作流

1. **读文档对应章节** —— 不要凭印象写。
2. **TaskCreate 建任务** —— 把工作拆成可独立验证的子任务。
3. **写一份最小可跑的版本** —— 不要先写完美的抽象。
4. **`idf.py build` 必须过** —— 不能过的代码不进 main。
5. **真机烧一次,确认行为** —— 屏幕/串口确认后再交付。
6. **更新相关文档** —— 改了什么功能就同步更新文档。
7. **提交** —— 见 §6 提交规范。

> **失败两次就停手。** 同一个错误改两次没修好,先回头读代码、读 errata、读 vendor demo。
> 不要在三十次小修改里循环。

---

## 5. 测试与验证

### 5.1 必须验证的项

- 修改 `port_bsp/` 任何文件:重新跑一次 boot splash。
- 修改 `wifi_bsp/`:确认能连上 `user_secrets.h` 里的 SSID,串口能拿到 IP。
- 修改 `partitions.csv` 或 `sdkconfig.defaults`:`idf.py erase-flash` 然后重烧。
- 修改 input/audio 组件后:跑一次 gamepad_test 和 snake 确认行为正常。

### 5.2 不能在主机上跑的东西

LCD / SD / 蓝牙 这种硬件相关的逻辑,**不要**写主机端单测,会撒谎。直接上板。

---

## 6. 提交 / 分支 / PR 规范

### 6.1 分支

- `main` —— 永远可烧、可跑。
- `feat/<short-name>` —— 新功能分支。
- `fix/<issue>` —— bug 修复。

### 6.2 提交信息

格式:
```
<type>: <subject>

<body 可选,解释为什么>
```

`type` 取值:`feat` / `fix` / `docs` / `refactor` / `chore` / `perf` / `test`。

**合并策略:**
- 大重构 / 新建组件 → **打包一个 PR,合并保留单提交**。
- bug fix → **每个 fix 一个独立提交**,便于回滚。

例:
```
feat: add BLE HID gamepad host + auto-reconnect
fix(input): latched button edges lost in bt_hid_input_poll
```

### 6.3 PR 要求

- PR 描述里必须有:**改了什么 / 为什么 / 怎么验证的(串口截图或屏幕拍照)**。
- 涉及硬件改动必须附烧录后真机的串口 log。
- 不要把无关改动塞进同一个 PR。

---

## 7. AI Agent 专属规则

> 如果你是 AI 协作者,以下是你独有的红线。

### 7.1 读再写

- **不要**对没读过的文件做修改建议。先 `Read`,再动。
- **不要**把"我猜这里大概是 X"当事实。验证它。

### 7.2 不要扩张作用域

- 用户让你修一个 bug,你只修这个 bug。不要顺手"清理"其他代码、补注释、加 type hint。
- 用户让你加一个简单功能,不要顺手做"为未来扩展性"的抽象。三行重复代码 > 提前抽象。
- 用户让你删一个文件,不要保留"兼容性 stub"。删干净。

### 7.3 不要绕过失败

- `idf.py build` 报错就修根因,不要 `--no-verify`、不要塞 `try/except: pass`、不要"暂时注释掉"。
- 跑了两次同样的修补还报同样错 → **停下来**,把假设拉出来重新验证,或问用户。

### 7.4 涉及破坏性操作必须先确认

需要先问用户的:`git push --force`、`git reset --hard`、`rm -rf`、改 `partitions.csv` 后 `erase-flash`、删除 `user_secrets.h` 等。
能本地复原的:直接做(改文件、build、烧录、跑测试)。

### 7.5 引用规范

- 提到代码位置用 `path/to/file.c:123` 格式。

### 7.6 提交触发

**只有用户明确说"提交 / commit / 提一个 PR"时才提交**。不要自作主张帮用户 `git commit`。

---

## 8. 常见坑(踩过的)

| 现象 | 根因 | 处理 |
|---|---|---|
| 文字 / 细线变成虚线点阵 | 用了中灰(100-200)描边,Bayer 抖动只有 ~50% 像素通过 | 改用纯黑(0)或深灰(32-64),详见 §3.1 显示设计规则 |
| LCD 上半部分丢 8 行 | ST7305 RAM 起始 column 偏移没设对 | 看 `port_bsp/st7305.c` 里 `col_start` 注释 |
| `malloc` 返回 NULL 但 PSRAM 没满 | 漏了 `CONFIG_SPIRAM_USE_MALLOC=y` | 检查 `sdkconfig.defaults` |
| BOOT 键读出来一直是 0 | 没开内部上拉 + 没消抖 | `gpio_set_pull_mode(GPIO_NUM_0, GPIO_PULLUP_ONLY)` + 软件 20 ms 消抖 |
| Wi-Fi 连不上 | `user_secrets.h` 没拷贝 / SSID 编码不对 | 用 `cp .example user_secrets.h` 后用 ASCII SSID 测试 |
| `idf.py build` 报 component not found | `CMakeLists.txt` 的 `REQUIRES` 漏了 | 加上对应组件名 |
| `input.pressed()` 检测不到按键 | BLE 回调在 poll 之前更新了 cur,导致 prev=cur 丢失边沿 | 检查 `bt_hid_input_poll()` 中 latched 处理 |

---

## 9. 文档维护

- 设计变更必须同步更新对应文档。**先改文档,再改代码**。
- README 随每个重大功能完成同步更新。
- 本文档(DEVELOPMENT-GUIDE.md)踩到的坑写进 §8。

---

> **本项目工程节奏:架构和产品定义由人主导,实现细节由 AI 完成,验收回归到人。**
> 不要打破这个节奏。
