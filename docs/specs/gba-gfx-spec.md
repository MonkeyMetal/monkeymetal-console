# GBA-Style Graphics Enhancements — 开发规范 (Spec)

> **状态:** 待开发 (Planned)
> **依赖:** gfx_engine v1.0+
> **目标版本:** v1.1.0+

## 概述

为 MonkeyMetal Console 的 gfx_engine 增加类似 GBA 风格的图形功能，使游戏开发者能实现更丰富的视觉效果 —— 精灵系统、瓦片地图、调色板动画、屏幕特效等。

## 动机

当前 gfx_engine 提供基础的绘图 API（line、rect、circle、text、pixel），但缺少：
- 精灵（sprite）系统 —— 带透明色的像素图渲染
- 瓦片地图（tilemap）—— 快速背景渲染
- 调色板（palette）—— 复用像素数据切换颜色方案
- 图层（layer）—— BG + 精灵层的合成
- 屏幕特效 —— 淡入淡出、屏幕震动、mosaic 效果

这些是 GBA 游戏的核心能力，也是限制当前游戏视觉表现力的瓶颈。

## 功能规格

### 1. 精灵系统（Sprite）

```lua
-- 加载精灵
local spr = gfx.sprite_load("/sdcard/games/my_game/sprites/player.mmi")

-- 绘制精灵（在指定位置）
gfx.sprite_draw(spr, x, y, {
    flip_h = false,    -- 水平翻转
    flip_v = false,    -- 垂直翻转
    palette = 0,       -- 调色板索引
    alpha = 255,       -- 透明度（0-255，Bayer 抖动近似）
    scale = 1.0,       -- 缩放（1.0 = 原始大小）
})

-- 精灵缓存（一次性提交，按 Y 排序绘制）
gfx.sprite_batch_begin()
gfx.sprite_batch_add(spr, x, y, flags)
gfx.sprite_batch_end()
```

**精灵文件格式 `.mmi`（MonkeyMetal Image）：**
```
[2 bytes]  width  (uint16 LE)
[2 bytes]  height (uint16 LE)
[1 byte]   bit_depth (1 = 1bpp)
[1 byte]   palette_count
[N bytes]  palette data (2 bytes per color: 16bpp grayscale)
[M bytes]  pixel data (row-major, 1bpp = 1 bit/pixel)
-- Transparent pixel: color index 0 in palette
```

### 2. 瓦片地图（Tilemap）

```lua
-- 加载 tileset
local tiles = gfx.tileset_load("/sdcard/games/my_game/tiles/map.tsx")

-- 设置瓦片地图（滚动背景）
local map = gfx.tilemap_create(tiles, map_width, map_height)
map:set_tile(tx, ty, tile_id)
map:set_scroll(x, y)  -- 视口偏移
gfx.tilemap_draw(map)

-- 元瓦片（meta-tile）—— 2×2 或 4×4 的组合
map:set_metatile(tx, ty, metatile_id)
```

**瓦片地图格式（`.tsx`）：**
```
[4 bytes] magic "MMTL"
[2 bytes] tile_width  (default 16)
[2 bytes] tile_height (default 16)
[2 bytes] tileset_cols
[2 bytes] tileset_rows
[1 byte]  bit_depth (1 = 1bpp)
[N bytes] tileset pixel data (row-major per tile)
```

### 3. 图层系统（Layers）

```lua
-- 创建图层（类似 GBA BG0-BG3 + OBJ）
local bg = gfx.layer_create("background", 0)  -- priority 0 = bottom
local fg = gfx.layer_create("foreground", 3)  -- priority 3 = top

-- 向图层绘制（图层有自己的帧缓冲，合成时 merge）
bg:clear(color)
bg:tilemap_draw(map)
fg:sprite_draw(spr, x, y)

-- 合成推屏
gfx.layers_flush()  -- BG0 → BG1 → BG2 → BG3 → sprites → LCD
```

### 4. 调色板动画（Palette Cycling）

```lua
-- 定义调色板
local pal = gfx.palette_create({
    {0, 0, 0},       -- index 0: 透明/黑色
    {64, 64, 64},    -- index 1: 深灰
    {128, 128, 128}, -- index 2: 中灰
    {255, 255, 255}, -- index 3: 白色
})

-- 调色板动画（循环切换 index 1-3 的值）
gfx.palette_cycle(pal, 1, 3, speed_ms)

-- 引用调色板绘制
gfx.sprite_draw(spr, x, y, { palette = pal })
```

### 5. 屏幕特效

```lua
-- 淡入淡出
gfx.fade_in(duration_ms)   -- 黑 → 正常
gfx.fade_out(duration_ms)  -- 正常 → 黑

-- 屏幕震动（受击反馈）
gfx.screen_shake(intensity, duration_ms)

-- Mosaic 效果（GBA 经典像素化过渡）
gfx.mosaic(block_size)  -- 0 = 正常, 4 = 4×4 像素块

-- 扫描线覆盖（复古 CRT 效果）
gfx.scanline_effect(enabled, line_height, alpha)

-- 窗口裁剪（类似 GBA WIN0/WIN1）
gfx.window_enable(win_id, x, y, w, h, invert)
```

### 6. 文本增强

```lua
-- 可变宽度字体
gfx.font_load("/sdcard/fonts/monkeyfont.mmf")

-- 文本对齐
gfx.text_align(s, x, y, color, { align = "center" | "left" | "right" })

-- 多行文本
gfx.text_box(s, x, y, w, h, color, { wrap = true })
```

## GBA 硬件对标

| GBA 特性 | MonkeyMetal 实现 |
| --- | --- |
| BG0-BG3 图层 (240×160) | 4 个图层 (400×300)，PSRAM 帧缓冲 |
| OBJ 精灵 (128 个，硬件缩放旋转) | 无限精灵（软件），支持翻转 |
| 调色板 (256 色/精灵表) | 16 色/精灵（1bpp 物理限制） |
| 256KB IWRAM (0 wait) | 240KB internal SRAM（快速池） |
| 32KB VRAM | PSRAM（~7MB 可用作"VRAM"） |
| Affine 变换 (BG2/BG3) | 软件实现旋转/缩放 |
| HDMA 特效 (窗口、混合) | 逐像素软件合成 |

## 性能目标

| 操作 | 目标耗时 (@160MHz) |
| --- | --- |
| 64×64 精灵绘制 (1bpp) | < 0.5 ms |
| 400×300 瓦片地图（16×16 tiles） | < 3 ms |
| 4 图层合成 (Bayer 抖动) | < 20 ms 总计（含 SPI DMA） |
| 100 精灵批量绘制 | < 5 ms |

## C 端实现要点

- `components/gfx_engine/` 扩展现有代码（非新组件）
- `gfx_sprite.c` — 精灵加载 (.mmi) + 绘制
- `gfx_tilemap.c` — 瓦片地图加载 (.tsx) + 绘制
- `gfx_layer.c` — 图层管理 + 合成
- `gfx_palette.c` — 调色板管理 + 动画
- `gfx_fx.c` — 屏幕特效（fade/shake/mosaic/scanline）
- `gfx_font.c` — 字体加载 (.mmf) + 增强渲染

## 精灵数据格式详解 (.mmi)

```
Offset  Size  Field
------  ----  -----
0x00    2     Magic "MI"
0x02    2     Width (pixels)
0x04    2     Height (pixels)
0x06    1     Bit depth (currently only 1)
0x07    1     Palette count (2-16)
0x08    2×N   Palette entries (16bpp gray: 0=black, 65535=white)
...     M     Pixel data (1bpp: 8 pixels/byte, row-major)
              Row bytes = ceil(width / 8)
```

## 参考资料

- GBA I/O Map (GBATEK)
- TONC GBA 图形编程教程
- GBA Pocky & Rocky / Drill Dozer 的调色板动画技巧
- Celeste 的屏幕震动参数研究
