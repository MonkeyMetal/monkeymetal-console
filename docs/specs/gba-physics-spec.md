# GBA-Style Physics Engine — 开发规范 (Spec)

> **状态:** 待开发 (Planned)
> **依赖:** gfx_engine v1.0+
> **目标版本:** v1.1.0+

## 概述

为 MonkeyMetal Console 实现一套类似 GBA 风格的 2D 物理引擎，使游戏开发者能用简单的 API 创建具有物理交互的游戏（平台跳跃、碰撞反弹、重力模拟等）。

## 动机

当前引擎只有基础的碰撞检测（`world:is_solid()`），缺乏：
- 刚体速度/加速度系统
- 重力模拟
- 弹性碰撞
- AABB 碰撞检测
- 平台跳跃物理（可变跳跃高度、土狼时间、输入缓冲）

参考 GBA 时代的标杆游戏（Super Mario Advance、Metroid Fusion），在有限硬件上实现流畅的物理反馈。

## 功能规格

### 1. 刚体系统（Rigid Body）

```lua
-- 创建刚体
body = physics.new_body(x, y, w, h, {
    mass = 1.0,       -- 质量（影响碰撞响应）
    gravity = 1.0,    -- 重力系数（0 = 无视重力）
    friction = 0.8,   -- 摩擦系数
    bounce = 0.0,     -- 弹性（0 = 完全非弹性，1 = 完全弹性）
    fixed = false,    -- true = 静态物体（墙体/地板）
})

-- 施加力
body:apply_force(fx, fy)
body:apply_impulse(ix, iy)
body:set_velocity(vx, vy)

-- 查询
local vx, vy = body:get_velocity()
local on_ground = body:on_ground()  -- 是否站在地面上
local on_wall = body:on_wall()      -- 是否贴墙
```

### 2. 碰撞检测

```lua
-- AABB vs AABB
local hit = physics.aabb_check(body1, body2)

-- AABB 扫描（防止高速穿透）
local hit, nx, ny, t = physics.aabb_sweep(body, other_bodies, dt)

-- 射线检测
local hit, x, y, nx, ny = physics.raycast(x1, y1, x2, y2, collision_group)
```

### 3. 平台跳跃专用功能

```lua
-- 土狼时间（离开平台后仍可跳跃的宽限期，默认 6 帧）
physics.set_coyote_time(frames)

-- 跳跃缓冲（落地前按下跳跃会缓存，默认 6 帧）
physics.set_jump_buffer(frames)

-- 可变跳跃高度（按住跳得更高）
physics.set_variable_jump(min_vy, max_vy, hold_duration)

-- 边缘修正（贴边时自动滑上平台，GBA 经典手感）
physics.set_edge_correction(enabled)
```

### 4. 重力与环境

```lua
-- 全局重力
physics.set_gravity(gx, gy)  -- 默认 (0, 600) 像素/秒²

-- 风力/水流区域
local zone = physics.add_zone(x, y, w, h, {
    gravity = {0, -200},  -- 区域内反向重力
    drag = 0.5,           -- 阻力
})
```

### 5. 物理层级/分组

```lua
-- 碰撞矩阵（类似 Unity Layer）
physics.set_collision_mask(group_a, group_b, enabled)

-- 触发器（检测碰撞但不阻挡）
body:set_trigger(true)
-- 触发回调
physics.on_trigger_enter = function(a, b) ... end
```

## 性能目标

| 指标 | 目标值 |
| --- | --- |
| 最大刚体数量 | 200 @ 30fps |
| 碰撞检测算法 | 空间哈希网格 |
| 单帧物理步进时间 | < 5ms |
| 内存占用 | < 50KB PSRAM |

## C 端实现要点

- `components/physics_engine/` — 新组件
- 使用 PSRAM 存储刚体池（对象复用避免 GC）
- 空间哈希网格大小：64×64 像素
- 每帧流程：`broadphase（空间哈希）→ narrowphase（AABB）→ 积分（Euler）→ 约束求解`

## Lua API 设计原则

- 保持与 GBA 时代一致的"手感优先"理念
- 数值使用定点数（16.16 fixed-point）避免浮点误差
- 提供默认常量（重力、摩擦等可直接使用）
- 所有回调在 `update()` 内触发（非异步）

## 参考资料

- GBA `tonc` 物理教程（定点数、碰撞响应）
- Celeste / TowerFall 的土狼时间 + 跳跃缓冲实现
- NES / SNES 平台跳跃游戏的输入处理
