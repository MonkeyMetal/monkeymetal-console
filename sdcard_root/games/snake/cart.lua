-- ============================================================
-- MonkeyMetal Console — Snake  v1.3
-- Controls: D-Pad (up/down/left/right) = direction
--           A / START = confirm on title / restart on game-over
--           BOOT key (system) = fallback A button
--           SELECT (LS click) or BOOT long-press = back to Launcher
-- Rules:    eat food to grow, wall or self = game over
-- Screen:   400×300 (gfx.W × gfx.H)
-- Audio:    audio.tone() for eat/die/start sounds
-- ============================================================

-- ── Grid config ──────────────────────────────────────────────
local CELL    = 12          -- pixels per cell
local COLS    = math.floor(gfx.W / CELL)   -- 33
local ROWS    = math.floor(gfx.H / CELL)   -- 25
local OX      = math.floor((gfx.W - COLS * CELL) / 2)  -- horizontal offset
local OY      = math.floor((gfx.H - ROWS * CELL) / 2)  -- vertical offset

-- ── Colors ───────────────────────────────────────────────────
local C_BG      = 255   -- white background
local C_GRID    = 220   -- very light gray grid lines
local C_WALL    = 0     -- black wall border
local C_SNAKE   = 0     -- black snake body
local C_HEAD    = 48    -- dark gray head (slightly lighter than body)
local C_FOOD    = 0     -- black food
local C_TEXT    = 0     -- black text

-- ── Direction vectors ─────────────────────────────────────────
-- dir index: 1=right 2=down 3=left 4=up
local DX = { 1,  0, -1,  0 }
local DY = { 0,  1,  0, -1 }

-- ── Game state ────────────────────────────────────────────────
local snake       -- array of {x,y}, [1]=head
local dir         -- current direction index (1–4)
local food        -- {x,y}
local score
local hi_score    -- persisted across sessions (in-memory for M3, storage.* in M4)
local alive
local move_timer  -- frames until next move
local MOVE_INTERVAL = 6   -- move every N frames (~5 moves/sec at 30fps)
local state       -- "play" | "over" | "title"
local blink_timer -- for game-over blink effect

-- Manual edge detection (input.pressed has C-side timing bug)
local up_prev, dn_prev, lt_prev, rt_prev = false, false, false, false

-- ── Helpers ───────────────────────────────────────────────────
local function rand_food()
    -- Place food not on snake
    local occupied = {}
    for _, seg in ipairs(snake) do
        occupied[seg.x * 1000 + seg.y] = true
    end
    local candidates = {}
    for x = 1, COLS do
        for y = 1, ROWS do
            if not occupied[x * 1000 + y] then
                candidates[#candidates + 1] = {x=x, y=y}
            end
        end
    end
    if #candidates == 0 then return nil end
    local i = (system.time_ms() * 2654435761) % #candidates + 1
    -- simple deterministic scatter using time
    i = math.floor(i) + 1
    if i > #candidates then i = 1 end
    return candidates[i]
end

local function cell_px(cx, cy)
    -- Returns top-left pixel of grid cell (cx, cy) (1-based)
    return OX + (cx - 1) * CELL, OY + (cy - 1) * CELL
end

local function start_game()
    -- Snake starts in the middle, length 3, heading right
    local mx = math.floor(COLS / 2)
    local my = math.floor(ROWS / 2)
    snake = {
        {x = mx,     y = my},
        {x = mx - 1, y = my},
        {x = mx - 2, y = my},
    }
    dir        = 1   -- right
    score      = 0
    alive      = true
    move_timer = MOVE_INTERVAL
    state      = "play"
    blink_timer = 0
    up_prev, dn_prev, lt_prev, rt_prev = false, false, false, false
    food = rand_food()
end

-- ── init / update / draw ──────────────────────────────────────
-- ── Sound effects ─────────────────────────────────────────────
local function sfx_eat()
    audio.tone(880, 80, 70)   -- high beep
end

local function sfx_die()
    -- Descending tones
    audio.tone(300, 120, 80)
    system.time_ms()           -- tiny gap (let tone finish)
    audio.tone(180, 200, 80)
end

local function sfx_start()
    audio.tone(440, 80, 60)
    audio.tone(660, 80, 60)
end

function init()
    system.log("Snake v1.1 init()")
    audio.master(80)
    hi_score = 0
    state    = "title"
end

function update()
    -- ── Title screen: wait for button ────────────────────────
    if state == "title" then
        if input.pressed("a") or input.pressed("start") then
            sfx_start()
            start_game()
        end
        return
    end

    -- ── Game Over: wait for button then restart ───────────────
    if state == "over" then
        blink_timer = blink_timer + 1
        if input.pressed("a") or input.pressed("start") then
            start_game()
        end
        return
    end

    -- ── Playing: handle D-Pad direction ──────────────────────
    -- D-Pad: direct direction control (no reverse allowed)
    if input.pressed("up")    and dir ~= 2 then dir = 4 end
    if input.pressed("down")  and dir ~= 4 then dir = 2 end
    if input.pressed("left")  and dir ~= 1 then dir = 3 end
    if input.pressed("right") and dir ~= 3 then dir = 1 end

    -- Move every MOVE_INTERVAL frames
    move_timer = move_timer - 1
    if move_timer > 0 then return end
    move_timer = MOVE_INTERVAL

    -- New head position
    local hx = snake[1].x + DX[dir]
    local hy = snake[1].y + DY[dir]

    -- Wall collision
    if hx < 1 or hx > COLS or hy < 1 or hy > ROWS then
        alive = false
        state = "over"
        if score > hi_score then hi_score = score end
        sfx_die()
        system.log("Game Over (wall) score=" .. score)
        return
    end

    -- Self collision (skip tail tip — it will move away)
    for i = 1, #snake - 1 do
        if snake[i].x == hx and snake[i].y == hy then
            alive = false
            state = "over"
            if score > hi_score then hi_score = score end
            sfx_die()
            system.log("Game Over (self) score=" .. score)
            return
        end
    end

    -- Advance snake
    table.insert(snake, 1, {x = hx, y = hy})

    -- Check food
    if food and hx == food.x and hy == food.y then
        score = score + 1
        sfx_eat()
        food  = rand_food()
        -- Speed up slightly every 5 points (min interval 2)
        if score % 5 == 0 and MOVE_INTERVAL > 2 then
            MOVE_INTERVAL = MOVE_INTERVAL - 1
        end
    else
        -- Remove tail (no food eaten)
        table.remove(snake)
    end
end

-- ── Draw helpers ─────────────────────────────────────────────
local function draw_cell(cx, cy, color, margin)
    margin = margin or 1
    local px, py = cell_px(cx, cy)
    gfx.rect(px + margin, py + margin,
             CELL - margin * 2, CELL - margin * 2,
             color, true)
end

local function draw_score_bar()
    -- Top bar: score left, hi-score right
    gfx.rect(0, 0, gfx.W, OY - 1, C_BG, true)
    -- Simple number rendering via filled/unfilled rects
    -- "S:" label via small block art
    local s = "SCORE " .. score
    local h = "BEST " .. hi_score
    -- Draw as dot-matrix using gfx.pixel (5×7 font too complex for M3,
    -- use horizontal bars as a placeholder readout)
    -- Score bar: filled rectangles proportional to score
    local bar_max = 30
    local bar_w   = math.min(score, bar_max) * 4
    gfx.rect(4, 2, bar_w + 1, OY - 4, 128, true)   -- gray fill
    gfx.rect(4, 2, bar_max * 4 + 1, OY - 4, C_WALL, false) -- outline
end

local function draw_grid()
    -- Light grid lines inside play area
    for x = 0, COLS do
        local px = OX + x * CELL
        gfx.line(px, OY, px, OY + ROWS * CELL, C_GRID)
    end
    for y = 0, ROWS do
        local py = OY + y * CELL
        gfx.line(OX, py, OX + COLS * CELL, py, C_GRID)
    end
end

local function draw_border()
    gfx.rect(OX - 2, OY - 2,
             COLS * CELL + 4, ROWS * CELL + 4,
             C_WALL, false)
    gfx.rect(OX - 1, OY - 1,
             COLS * CELL + 2, ROWS * CELL + 2,
             C_WALL, false)
end

function draw()
    if state == "title" then
        gfx.clear(C_BG)

        -- Title box
        gfx.rect(60, 80, 280, 140, C_WALL, false)
        gfx.rect(62, 82, 276, 136, C_WALL, false)

        -- "SNAKE" as big pixel blocks (3×3 per pixel, 5-wide letters)
        -- Draw a simple snake silhouette instead
        local sx, sy = 120, 110
        for i = 0, 7 do
            gfx.rect(sx + i * 14, sy + math.floor(math.sin(i) * 10 + 10),
                     10, 10, C_SNAKE, true)
        end

        -- Subtitle bar
        gfx.rect(100, 160, 200, 20, C_SNAKE, true)
        gfx.rect(102, 162, 196, 16, C_BG, true)

        -- "PRESS BOOT" hint via dashes
        for i = 0, 9 do
            gfx.rect(110 + i * 18, 167, 12, 5, C_SNAKE, true)
        end

        -- Best score indicator bar
        if hi_score > 0 then
            local bar = math.min(hi_score * 8, 200)
            gfx.rect(100, 195, bar, 8, 64, true)
            gfx.rect(100, 195, 200, 8, C_WALL, false)
        end

        gfx.flip()
        return
    end

    -- ── Play / Game Over ──────────────────────────────────────
    gfx.clear(C_BG)
    draw_grid()
    draw_border()
    draw_score_bar()

    -- Food (blinking dot)
    if food then
        local ft = math.floor(system.time_ms() / 300) % 2
        if ft == 0 then
            draw_cell(food.x, food.y, C_FOOD, 2)
        else
            draw_cell(food.x, food.y, 80, 2)
        end
    end

    -- Snake body
    for i = 2, #snake do
        draw_cell(snake[i].x, snake[i].y, C_SNAKE, 1)
    end

    -- Snake head (slightly different shade)
    draw_cell(snake[1].x, snake[1].y, C_HEAD, 0)
    -- Eye on head
    local epx, epy = cell_px(snake[1].x, snake[1].y)
    local ex = epx + (DX[dir] >= 0 and CELL - 3 or 2)
    local ey = epy + (DY[dir] >= 0 and CELL - 3 or 2)
    gfx.rect(ex, ey, 3, 3, C_BG, true)

    -- Game Over overlay
    if state == "over" then
        -- Dim overlay (checkerboard via rect pattern)
        local blink = math.floor(blink_timer / 15) % 2 == 0
        if blink then
            -- Flash border
            gfx.rect(OX - 3, OY - 3,
                     COLS * CELL + 6, ROWS * CELL + 6,
                     0, false)
        end

        -- Game over box
        gfx.rect(100, 100, 200, 100, C_BG, true)
        gfx.rect(100, 100, 200, 100, C_WALL, false)
        gfx.rect(102, 102, 196, 96, C_WALL, false)

        -- Score bar inside box
        local bar = math.min(score * 6, 170)
        gfx.rect(115, 130, bar, 12, 0, true)
        gfx.rect(115, 130, 170, 12, C_WALL, false)

        -- "BOOT to retry" hint dots
        for i = 0, 7 do
            gfx.rect(120 + i * 20, 160, 14, 6, C_SNAKE, true)
        end
    end

    gfx.flip()
end
