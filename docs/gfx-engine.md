# Graphics Engine (gfx_engine)

> **组件路径:** `components/gfx_engine/`
> **依赖:** `port_bsp`(LCD 推屏)
> **状态:** v1.0.0,真机验收通过

---

## 1. 整体管线

```
游戏代码调用绘图 API
        │
        ▼
  16bpp 帧缓冲 (PSRAM)
  400 × 300 × 2B = 240 KB
  EXT_RAM_BSS_ATTR
        │
    gfx_flush()
        │
        ▼
  Bayer 4×4 有序抖动
  gray8 vs threshold → 1bit
        │
        ▼
  1bpp 输出缓冲 (PSRAM)
  400 × 300 / 8 = 15,000 B
  ST7305 landscape 打包格式
        │
  port_bsp_lcd_flush()
        │
        ▼
  ST7305 反射屏
  SPI DMA @10 MHz
  ~30 ms / 帧
```

**颜色模型:** 16bpp 灰阶(0=黑, 65535=白)。游戏用中间值表示灰色,抖动时转成点阵 1bpp。ST7305 物理上只有黑/白两态。

---

## 2. 文件结构

| 文件 | 职责 |
|---|---|
| `gfx_engine.h` | 公共 API 声明,唯一对外头文件 |
| `gfx_framebuffer.c` | 帧缓冲分配、`gfx_init` / `gfx_clear` / `gfx_pixel` |
| `gfx_draw.c` | 绘图原语:line / rect / circle |
| `gfx_text.c` | 8×8 位图字体渲染,ASCII 32–126 |
| `gfx_font8x8.h` | 8×8 字体点阵数据 |
| `gfx_bayer.c` | Bayer 抖动 + ST7305 像素打包 + `gfx_flush` |

---

## 3. 公共 API

### 3.1 初始化

```c
esp_err_t gfx_init(void);
```

- 把帧缓冲清为白色(`GFX_WHITE`)
- 必须在 LCD `RLCD_Init()` 之后调用
- 只调一次

### 3.2 颜色常量与宏

```c
#define GFX_BLACK        0x0000
#define GFX_WHITE        0xFFFF
#define GFX_GRAY(x)      ((uint16_t)((x) * 257))   // x: 0–255
```

`GFX_GRAY(128)` ≈ 中灰,抖动后约 50% 密度点阵。

### 3.3 清屏

```c
void gfx_clear(gfx_color_t color);
```

整帧填充同一颜色,O(width×height)。

### 3.4 像素

```c
void gfx_pixel(int x, int y, gfx_color_t color);
```

越界自动裁剪,不报错。坐标系:左上角 (0,0),x 向右,y 向下。

### 3.5 直线

```c
void gfx_line(int x1, int y1, int x2, int y2, gfx_color_t color);
```

Bresenham 算法,端点包含,越界自动裁剪。

### 3.6 矩形

```c
void gfx_rect(int x, int y, int w, int h, gfx_color_t color, bool fill);
```

- `(x, y)` 是左上角
- `fill=true` 实心填充,`fill=false` 只画 4 条边
- `w=0` 或 `h=0` 什么也不画

### 3.7 圆

```c
void gfx_circle(int cx, int cy, int r, gfx_color_t color, bool fill);
```

- 空心:midpoint 算法,精确 8 对称
- 实心:扫描线 + `sqrt`,每行填一段

### 3.8 文字

```c
void gfx_text(const char *s, int x, int y, gfx_color_t color);
void gfx_text_char(char ch, int x, int y, gfx_color_t color);
```

- 8×8 位图字体,ASCII 32–126
- 8 像素宽,8 像素高,无间距
- `gfx_text` 自动换行到 `(x, y+8)`,不处理窗口裁剪
- `gfx_text_char` 画单个字符

### 3.9 推屏

```c
esp_err_t gfx_flush(void);
```

- 对帧缓冲做 Bayer 抖动,输出 1bpp
- 调用 `port_bsp_lcd_flush()` 推 SPI DMA
- **阻塞**直到 DMA 完成,约 30 ms
- 每帧最多调一次;调完才能开始写下一帧

---

## 4. 典型游戏帧循环

```c
while (1) {
    // 1. 清帧
    gfx_clear(GFX_WHITE);

    // 2. 画场景
    gfx_rect(player_x, player_y, 16, 16, GFX_BLACK, true);
    gfx_circle(enemy_x, enemy_y, 8, GFX_GRAY(64), false);
    gfx_line(0, 200, 400, 200, GFX_GRAY(128));
    gfx_text("SCORE 42", 10, 10, GFX_BLACK);

    // 3. 推屏(阻塞 ~30 ms)
    gfx_flush();
}
```

---

## 5. Bayer 4×4 抖动原理

```
阈值矩阵 (0–255)      像素灰度 > 对应阈值 → 白点
┌────┬────┬────┬────┐
│  0 │128 │ 32 │160 │
│192 │ 64 │224 │ 96 │
│ 48 │176 │ 16 │144 │
│240 │112 │208 │ 80 │
└────┴────┴────┴────┘
矩阵在 x、y 方向各自以 4 为周期平铺到整屏。
```

**感知效果:**
- `GFX_BLACK (0)` → 全黑点阵
- `GFX_GRAY(64)` → 约 25% 白点
- `GFX_GRAY(128)` → 约 50% 棋盘
- `GFX_GRAY(192)` → 约 75% 白点
- `GFX_WHITE (65535)` → 全白点阵

**代码路径** (`gfx_bayer.c`):
```
gray16 = framebuffer[y*W+x]
gray8  = gray16 >> 8
threshold = bayer_matrix[y&3][x&3]
pixel = (gray8 > threshold) ? 1 : 0
```

---

## 6. ST7305 1bpp 像素打包

ST7305 landscape 模式下,内存排布**不是**按行优先:

```
每字节覆盖: 2列 × 4行 的块
byte_index = (x/2) * (HEIGHT/4) + (inv_y/4)
bit_index  = 7 - ((inv_y%4)*2 + x%2)
inv_y      = HEIGHT-1-y   ← ST7305 从底部向上扫描
```

**为什么 inv_y:**  ST7305 的行扫描方向是物理底→顶。屏幕坐标系 y=0 在顶部,所以需要翻转。

**缓冲区大小:** `400×300/8 = 15,000 字节`。超出此范围的写入会破坏 PSRAM 中紧邻的其他变量。

---

## 7. 内存布局

| 对象 | 位置 | 大小 |
|---|---|---|
| `s_framebuffer` (16bpp) | PSRAM (`EXT_RAM_BSS_ATTR`) | 240,000 B ≈ 234 KB |
| `s_output_1bpp` | PSRAM (`EXT_RAM_BSS_ATTR`) | 15,000 B ≈ 15 KB |
| 两者合计 | PSRAM | ~249 KB |

**限制:** ESP32-S3 PSRAM 总量 8 MB,上面两个只占 ~3%,非常充裕。但 SPI DMA 只能访问 internal RAM。`port_bsp_lcd_flush` 的 DMA 传输由 `esp_lcd` 驱动负责,它内部会处理 PSRAM→internal 的搬运(或使用 `MALLOC_CAP_DMA` 的缓冲)。如果遇到花屏,先查 `DispBuffer` 的分配标志。

---

## 8. 性能参考

| 操作 | 耗时(实测 @160 MHz) |
|---|---|
| `gfx_clear()` | ~0.6 ms |
| `gfx_flush()` Bayer 抖动部分 | ~18 ms |
| `gfx_flush()` SPI DMA 传输 | ~12 ms |
| `gfx_flush()` 总计 | ~30 ms |
| 理论帧率上限 | ~33 fps |

实际游戏帧率受绘制复杂度影响。`gfx_circle(fill=true)` 调用 `sqrt`,对大圆有额外开销。

---

## 9. Lua 绑定

C API 已全部暴露给 Lua 运行时:

```lua
-- 对应关系
gfx.clear(c)             → gfx_clear(GFX_GRAY(c))
gfx.pixel(x, y, c)       → gfx_pixel(x, y, GFX_GRAY(c))
gfx.line(x1,y1,x2,y2,c)  → gfx_line(...)
gfx.rect(x,y,w,h,c,fill) → gfx_rect(...)
gfx.circle(cx,cy,r,c,f)  → gfx_circle(...)
gfx.text(s, x, y, c)     → gfx_text(s, x, y, GFX_GRAY(c))
gfx.flip()               → gfx_flush()
```

Lua 层的 `c` 参数约定 0–255 灰阶,binding 层统一转 `GFX_GRAY(c)`。

---

## 10. 已知问题 / 踩过的坑

| 现象 | 根因 | 状态 |
|---|---|---|
| 屏幕上下翻转 | 忘记 `inv_y = HEIGHT-1-y` | 已修(真机验收通过) |
| 灰色区域出现水平条纹 | Bayer 矩阵用行地址模 4 错误 | 已修 |
| `gfx_flush` 后花屏 | `s_output_1bpp` 行优先打包,不符合 ST7305 landscape 格式 | 已修,见 §6 |
| 大实心圆边缘锯齿 | `sqrt` 整数截断 | 已知,可接受 |
