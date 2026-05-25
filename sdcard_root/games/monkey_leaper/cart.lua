-- ============================================================
-- MonkeyMetal Console — Monkey Leaper v1.0
-- Controls: D-Pad (left/right) = horizontal move
--           D-Pad (up) or A button = jump
--           START = start / restart / confirm
--           SELECT = exit back to launcher
-- Rules:    Collect bananas, avoid spikes, and climb to the goal!
-- Screen:   400×300, 1bpp
-- ============================================================

-- ── Color Definitions ────────────────────────────────────────
local C_BG    = 255   -- Pure White Background
local C_BLACK = 0     -- Pure Black Foreground

-- ── Physical Dimensions ──────────────────────────────────────
local TILE_SIZE = 20
local COLS      = 20
local ROWS      = 15

-- ── Level Templates ──────────────────────────────────────────
-- Legend:
-- . = Empty Space
-- # = Solid Ground (with brick patterns)
-- B = Banana (10 points)
-- G = Goal Vine/Flag (triggers next level)
-- ^ = Spikes (death hazard)
-- T = Trees (background decoration, non-solid)
-- P = Player Start (dynamically converted to spawn and emptied)
local LEVEL_TEMPLATES = {
    -- Level 1: The Climbing Vine
    {
        "....................",
        "..................G.",
        "..................#.",
        "...............###..",
        ".............##.....",
        "...........##.......",
        ".........##..B......",
        "......###....#......",
        "....##..............",
        "...#....B...........",
        "..#.....#...B.......",
        ".#..........#.......",
        "T...................",
        "T..P...B.........B..",
        "####################"
    },
    -- Level 2: Spike Leap
    {
        "..................G.",
        "..................#.",
        "..............####..",
        "............##......",
        "..........##........",
        "........##...B.B....",
        "......##.....###....",
        "....##..............",
        "....#....B..........",
        ".........#....B.....",
        "....P.........#.....",
        "#############.......",
        "............#^^^^#..",
        "............######..",
        "####################"
    },
    -- Level 3: The Canopy Hunt
    {
        "G...................",
        "#...................",
        "##...B....B....B....",
        "..#########.########",
        "....................",
        ".....B...B...B......",
        "....###########.....",
        "....................",
        "B.B.............B.B.",
        "###....P....^...####",
        "......#####.###.....",
        "....##.........##...",
        "T.##.............##.",
        "T#....^^^^.^^^^....#",
        "####################"
    }
}

-- ── Physics Constants ────────────────────────────────────────
local GRAVITY           = 0.45
local TERMINAL_VELOCITY = 8.0
local JUMP_VELOCITY     = -7.5
local ACCEL             = 0.4
local FRICTION          = 0.8
local MAX_SPEED         = 3.5

-- ── Bounding Box ─────────────────────────────────────────────
local pw = 14  -- Player width
local ph = 18  -- Player height

-- ── Game State ───────────────────────────────────────────────
local state                    -- "title" | "play" | "complete_fanfare" | "gameover" | "victory"
local score
local lives
local current_level
local frame_count
local fanfare_timer
local bananas_collected
local total_bananas_collected

-- Player parameters
local px, py
local vx, vy
local grounded
local facing

-- Particles
local particles = {}

-- Audio sequencer queue
local audio_seq = {}
local audio_seq_idx = 1
local audio_timer = 0

-- ── Audio System & Music Sequencer ───────────────────────────
local function play_melody(notes)
    audio_seq = notes
    audio_seq_idx = 1
    audio_timer = 0
end

local function update_audio_sequence()
    if #audio_seq == 0 then return end
    audio_timer = audio_timer - 1
    if audio_timer <= 0 then
        local note = audio_seq[audio_seq_idx]
        if note then
            audio.tone(note.freq, note.dur, 70)
            audio_timer = math.floor(note.delay)
            audio_seq_idx = audio_seq_idx + 1
        else
            audio_seq = {}
        end
    end
end

local function sfx_jump()
    audio.tone(523, 60, 60)
end

local function sfx_eat()
    audio.tone(880, 50, 70)
end

local function sfx_die()
    audio.tone(180, 250, 80)
end

local function sfx_complete()
    play_melody({
        {freq = 523,  dur = 80,  delay = 6},   -- C5
        {freq = 659,  dur = 80,  delay = 6},   -- E5
        {freq = 784,  dur = 80,  delay = 6},   -- G5
        {freq = 1047, dur = 250, delay = 15}   -- C6
    })
end

-- ── Particles System ─────────────────────────────────────────
local function spawn_particles(cx, cy, count)
    for i = 1, count do
        local angle = ((system.time_ms() * 1103515245 + 12345) % 65536) / 65536 * math.pi * 2
        local speed = ((system.time_ms() * 2654435761) % 1000) / 1000 * 3 + 1
        table.insert(particles, {
            x = cx,
            y = cy,
            vx = math.cos(angle) * speed,
            vy = math.sin(angle) * speed,
            life = math.floor(((system.time_ms() * 1664525) % 15) + 15)
        })
    end
end

local function update_particles()
    for i = #particles, 1, -1 do
        local p = particles[i]
        p.x = p.x + p.vx
        p.y = p.y + p.vy
        p.life = p.life - 1
        if p.life <= 0 then
            table.remove(particles, i)
        end
    end
end

-- ── Collision Detection & Resolution ─────────────────────────
local current_level_map = {}
local start_px, start_py = 0, 0

local function tile_is_solid(c, r)
    if c < 1 or c > COLS then return true end -- Out of bounds sideways is solid wall
    if r < 1 then return false end             -- Sky is not solid
    if r > ROWS then return false end           -- Bottom pit (falls through to death)
    return current_level_map[r][c] == 1
end

local function collide_with_solid(x, y)
    local c1 = math.floor(x / TILE_SIZE) + 1
    local c2 = math.floor((x + pw) / TILE_SIZE) + 1
    local r1 = math.floor(y / TILE_SIZE) + 1
    local r2 = math.floor((y + ph) / TILE_SIZE) + 1

    for r = r1, r2 do
        for c = c1, c2 do
            if tile_is_solid(c, r) then
                return true
            end
        end
    end
    return false
end

-- ── Level Transitions & Helper Functions ─────────────────────
local function respawn_player()
    px = start_px
    py = start_py
    vx = 0
    vy = 0
    grounded = false
    facing = 1
end

local function load_level(num)
    current_level = num
    current_level_map = {}
    bananas_collected = 0

    local template = LEVEL_TEMPLATES[num]
    for r = 1, ROWS do
        current_level_map[r] = {}
        local line = template[r]
        for c = 1, COLS do
            local char = line:sub(c, c)
            local tile = 0
            if char == "#" then
                tile = 1
            elseif char == "B" then
                tile = 2
            elseif char == "G" then
                tile = 3
            elseif char == "^" then
                tile = 4
            elseif char == "T" then
                tile = 5
            elseif char == "P" then
                start_px = (c - 1) * TILE_SIZE + 3
                start_py = (r - 1) * TILE_SIZE + 1
                tile = 0
            end
            current_level_map[r][c] = tile
        end
    end
    respawn_player()
end

local function die()
    sfx_die()
    spawn_particles(px + pw / 2, py + ph / 2, 12)
    lives = lives - 1
    if lives <= 0 then
        state = "gameover"
    else
        respawn_player()
    end
end

local function level_complete()
    sfx_complete()
    spawn_particles(px + pw / 2, py + ph / 2, 15)
    state = "complete_fanfare"
    fanfare_timer = 90  -- ~3 seconds of fanfare
end

local function start_game()
    score = 0
    lives = 3
    total_bananas_collected = 0
    load_level(1)
    state = "play"
end

-- ── Interactive Overlaps (Sensors) ───────────────────────────
local function handle_sensor_collisions()
    local c1 = math.floor(px / TILE_SIZE) + 1
    local c2 = math.floor((px + pw) / TILE_SIZE) + 1
    local r1 = math.floor(py / TILE_SIZE) + 1
    local r2 = math.floor((py + ph) / TILE_SIZE) + 1

    for r = r1, r2 do
        for c = c1, c2 do
            if r >= 1 and r <= ROWS and c >= 1 and c <= COLS then
                local tile = current_level_map[r][c]
                if tile == 2 then
                    -- Collect Banana
                    current_level_map[r][c] = 0
                    score = score + 10
                    bananas_collected = bananas_collected + 1
                    sfx_eat()
                    spawn_particles((c - 1) * TILE_SIZE + 10, (r - 1) * TILE_SIZE + 10, 4)
                elseif tile == 3 then
                    -- Collect Goal
                    level_complete()
                    return
                elseif tile == 4 then
                    -- Touch Spikes
                    die()
                    return
                end
            end
        end
    end

    -- Check bottom pit bounds
    if py > 300 then
        die()
    end
end

-- ── Lifecycle: Init / Update / Draw ──────────────────────────
function init()
    system.log("Monkey Leaper game initializing")
    audio.master(80)
    state = "title"
    frame_count = 0
end

function update()
    frame_count = frame_count + 1

    -- Exit to launcher on SELECT button
    if input.pressed("select") then
        system.run_cart("/sdcard/system/launcher")
        return
    end

    -- Title Screen state
    if state == "title" then
        if input.pressed("start") or input.pressed("a") then
            start_game()
        end
        return
    end

    -- Game Over / Victory state
    if state == "gameover" or state == "victory" then
        if input.pressed("start") or input.pressed("a") then
            start_game()
        end
        return
    end

    -- Complete Fanfare state
    if state == "complete_fanfare" then
        fanfare_timer = fanfare_timer - 1
        update_particles()
        update_audio_sequence()
        if fanfare_timer <= 0 then
            total_bananas_collected = total_bananas_collected + bananas_collected
            if current_level < #LEVEL_TEMPLATES then
                load_level(current_level + 1)
                state = "play"
            else
                state = "victory"
            end
        end
        return
    end

    -- ── Main Gameplay Updates ─────────────────────────────────
    update_audio_sequence()
    update_particles()

    -- Apply gravity
    vy = vy + GRAVITY
    if vy > TERMINAL_VELOCITY then
        vy = TERMINAL_VELOCITY
    end

    -- Read horizontal input controls
    if input.down("left") then
        vx = vx - ACCEL
        if vx < -MAX_SPEED then vx = -MAX_SPEED end
        facing = -1
    elseif input.down("right") then
        vx = vx + ACCEL
        if vx > MAX_SPEED then vx = MAX_SPEED end
        facing = 1
    else
        -- Slide decay
        vx = vx * FRICTION
        if math.abs(vx) < 0.05 then vx = 0 end
    end

    -- Jump control
    if (input.pressed("up") or input.pressed("a")) and grounded then
        vy = JUMP_VELOCITY
        grounded = false
        sfx_jump()
    end

    -- Horizontal collision resolution step
    px = px + vx
    if collide_with_solid(px, py) then
        if vx > 0 then
            local r_col = math.floor((px + pw) / TILE_SIZE) + 1
            px = (r_col - 1) * TILE_SIZE - pw - 0.01
        elseif vx < 0 then
            local l_col = math.floor(px / TILE_SIZE) + 1
            px = l_col * TILE_SIZE + 0.01
        end
        vx = 0
    end

    -- Vertical collision resolution step
    py = py + vy
    grounded = false
    if collide_with_solid(px, py) then
        if vy > 0 then
            local b_row = math.floor((py + ph) / TILE_SIZE) + 1
            py = (b_row - 1) * TILE_SIZE - ph - 0.01
            vy = 0
            grounded = true
        elseif vy < 0 then
            local t_row = math.floor(py / TILE_SIZE) + 1
            py = t_row * TILE_SIZE + 0.01
            vy = 0
        end
    end

    -- Verify standing support if not already flagged grounded
    if not grounded then
        if collide_with_solid(px, py + 1) then
            grounded = true
        end
    end

    -- Assess collectibles and death interactions
    handle_sensor_collisions()
end

-- ── Drawing Helpers ──────────────────────────────────────────
local function draw_heart(hx, hy)
    gfx.rect(hx, hy, 2, 2, C_BLACK, true)
    gfx.rect(hx + 4, hy, 2, 2, C_BLACK, true)
    gfx.rect(hx - 1, hy + 2, 8, 2, C_BLACK, true)
    gfx.rect(hx, hy + 4, 6, 2, C_BLACK, true)
    gfx.rect(hx + 2, hy + 6, 2, 2, C_BLACK, true)
end

local function draw_monkey_sprite(mx, my, face_dir, motion, frame)
    local x = math.floor(mx)
    local y = math.floor(my)

    -- Head
    gfx.circle(x + 7, y + 5, 4, C_BLACK, true)
    -- Ears
    gfx.circle(x + 7 - 5 * face_dir, y + 4, 2, C_BLACK, true)
    gfx.circle(x + 7 + 5 * face_dir, y + 4, 2, C_BLACK, true)
    -- Inner ear
    gfx.pixel(x + 7 + 5 * face_dir, y + 4, C_BG)
    -- Face snout (white patch on 1bpp screen for visibility)
    gfx.rect(x + 7 + (face_dir > 0 and 1 or -3), y + 5, 2, 2, C_BG, true)
    gfx.pixel(x + 7 + face_dir, y + 4, C_BG) -- eye
    -- Torso
    gfx.rect(x + 3, y + 10, 8, 5, C_BLACK, true)
    -- Arms (bobbing based on movement style)
    local arm_y = y + 11
    local bob = (motion == "run") and math.floor(math.sin(frame * 0.4) * 3) or 0
    gfx.line(x + 7, arm_y, x + 7 + 5 * face_dir, arm_y + bob, C_BLACK)
    -- Legs
    local leg_bob = (motion == "run") and math.floor(math.cos(frame * 0.4) * 2) or 0
    gfx.line(x + 4, y + 15, x + 4, y + 17 + (leg_bob > 0 and leg_bob or 0), C_BLACK)
    gfx.line(x + 10, y + 15, x + 10, y + 17 + (leg_bob < 0 and -leg_bob or 0), C_BLACK)
    -- Tail
    local tail_wave = math.floor(math.sin(frame * 0.15) * 2)
    gfx.line(x + 3, y + 12, x + 7 - 8 * face_dir, y + 8 + tail_wave, C_BLACK)
end

local function draw_level()
    for r = 1, ROWS do
        for c = 1, COLS do
            local tile = current_level_map[r][c]
            local tx = (c - 1) * TILE_SIZE
            local ty = (r - 1) * TILE_SIZE

            if tile == 1 then
                -- Brick Pattern Outline
                gfx.rect(tx, ty, TILE_SIZE, TILE_SIZE, C_BLACK, false)
                gfx.rect(tx + 1, ty + 1, 18, 18, C_BLACK, false)
                -- Brick texture cuts
                gfx.line(tx + 2, ty + 10, tx + 17, ty + 10, C_BLACK)
                gfx.line(tx + 10, ty + 2, tx + 10, ty + 9, C_BLACK)
                gfx.line(tx + 5, ty + 11, tx + 5, ty + 17, C_BLACK)
                gfx.line(tx + 15, ty + 11, tx + 15, ty + 17, C_BLACK)
            elseif tile == 2 then
                -- Curved banana shape
                local bx, by = tx + 4, ty + 4
                gfx.line(bx + 2, by + 10, bx + 10, by + 2, C_BLACK)
                gfx.line(bx + 3, by + 10, bx + 10, by + 3, C_BLACK)
                gfx.line(bx + 2, by + 9, bx + 9, by + 2, C_BLACK)
                gfx.line(bx + 10, by + 2, bx + 12, by, C_BLACK)
            elseif tile == 3 then
                -- Goal flagpole and banner
                gfx.rect(tx + 9, ty, 2, 20, C_BLACK, true)
                gfx.rect(tx + 11, ty + 2, 8, 6, C_BLACK, false)
                gfx.line(tx + 11, ty + 5, tx + 18, ty + 5, C_BLACK)
            elseif tile == 4 then
                -- Spikes
                local sy = ty + 19
                gfx.line(tx, sy, tx + 3, ty + 8, C_BLACK)
                gfx.line(tx + 3, ty + 8, tx + 6, sy, C_BLACK)
                gfx.line(tx + 7, sy, tx + 10, ty + 8, C_BLACK)
                gfx.line(tx + 10, ty + 8, tx + 13, sy, C_BLACK)
                gfx.line(tx + 14, sy, tx + 17, ty + 8, C_BLACK)
                gfx.line(tx + 17, ty + 8, tx + 19, sy, C_BLACK)
                -- Solid interior fills for spike triangles
                for py = ty + 9, sy do
                    local w = math.floor((py - ty - 8) / 1.5)
                    gfx.line(tx + 3 - w, py, tx + 3 + w, py, C_BLACK)
                    gfx.line(tx + 10 - w, py, tx + 10 + w, py, C_BLACK)
                    gfx.line(tx + 17 - w, py, tx + 17 + w, py, C_BLACK)
                end
            elseif tile == 5 then
                -- Transparent decorative background trees
                gfx.rect(tx + 9, ty + 10, 2, 10, C_BLACK, true)
                gfx.circle(tx + 10, ty + 8, 6, C_BLACK, false)
                gfx.circle(tx + 6, ty + 10, 4, C_BLACK, false)
                gfx.circle(tx + 14, ty + 10, 4, C_BLACK, false)
            end
        end
    end
end

local function draw_particles()
    for _, p in ipairs(particles) do
        local sz = p.life > 10 and 2 or 1
        gfx.rect(math.floor(p.x - sz / 2), math.floor(p.y - sz / 2), sz, sz, C_BLACK, true)
    end
end

local function draw_title_screen()
    gfx.clear(C_BG)
    -- Frame borders
    gfx.rect(10, 10, 380, 280, C_BLACK, false)
    gfx.rect(12, 12, 376, 276, C_BLACK, false)

    -- Corner structures
    gfx.rect(30, 240, 4, 30, C_BLACK, true)
    gfx.circle(32, 230, 12, C_BLACK, false)
    gfx.rect(360, 240, 4, 30, C_BLACK, true)
    gfx.circle(362, 230, 12, C_BLACK, false)

    -- Center piece
    gfx.rect(140, 180, 120, 15, C_BLACK, false)
    gfx.line(140, 187, 260, 187, C_BLACK)

    local bx, by = 194, 155
    gfx.line(bx + 2, by + 10, bx + 10, by + 2, C_BLACK)
    gfx.line(bx + 3, by + 10, bx + 10, by + 3, C_BLACK)
    gfx.line(bx + 2, by + 9, bx + 9, by + 2, C_BLACK)
    gfx.line(bx + 10, by + 2, bx + 12, by, C_BLACK)

    -- Banner Text
    gfx.text("MONKEY LEAPER", 150, 60, C_BLACK)
    gfx.line(140, 75, 260, 75, C_BLACK)
    gfx.line(140, 77, 260, 77, C_BLACK)
    gfx.text("A Retro 1bpp Platformer", 115, 95, C_BLACK)

    -- Prompt Blinker
    if math.floor(frame_count / 15) % 2 == 0 then
        gfx.text("PRESS START TO PLAY", 125, 220, C_BLACK)
    end
    gfx.text("D-PAD: Move & Jump", 135, 250, C_BLACK)
    gfx.text("SELECT: Exit to Launcher", 115, 265, C_BLACK)
end

local function draw_game_over_screen()
    gfx.clear(C_BG)
    gfx.rect(50, 50, 300, 200, C_BLACK, false)
    gfx.rect(52, 52, 296, 196, C_BLACK, false)

    gfx.text("GAME OVER", 165, 80, C_BLACK)
    gfx.line(140, 95, 260, 95, C_BLACK)

    gfx.text("Score achieved: " .. score, 145, 130, C_BLACK)
    gfx.text("Level reached: " .. current_level, 145, 150, C_BLACK)

    if math.floor(frame_count / 15) % 2 == 0 then
        gfx.text("PRESS START TO RETRY", 125, 200, C_BLACK)
    end
    gfx.text("SELECT: Exit to Launcher", 115, 220, C_BLACK)
end

local function draw_victory_screen()
    gfx.clear(C_BG)
    gfx.rect(30, 30, 340, 240, C_BLACK, false)
    gfx.rect(32, 32, 336, 236, C_BLACK, false)

    gfx.text("VICTORY!", 170, 60, C_BLACK)
    gfx.line(150, 75, 250, 75, C_BLACK)

    gfx.text("The Monkey has grabbed all the Bananas!", 100, 110, C_BLACK)
    gfx.text("FINAL SCORE: " .. score, 140, 150, C_BLACK)
    gfx.text("BANANAS: " .. total_bananas_collected, 150, 170, C_BLACK)

    local bob = math.floor(math.sin(frame_count * 0.15) * 4)
    draw_monkey_sprite(192, 190 + bob, 1, "run", frame_count)

    if math.floor(frame_count / 15) % 2 == 0 then
        gfx.text("PRESS START TO RETRY", 125, 240, C_BLACK)
    end
end

function draw()
    if state == "title" then
        draw_title_screen()
        gfx.flip()
        return
    end

    if state == "gameover" then
        draw_game_over_screen()
        gfx.flip()
        return
    end

    if state == "victory" then
        draw_victory_screen()
        gfx.flip()
        return
    end

    -- Play field clear
    gfx.clear(C_BG)

    -- Draw environment layout
    draw_level()

    -- Draw user player avatar
    local move_type = (vx == 0) and "idle" or "run"
    draw_monkey_sprite(px, py, facing, move_type, frame_count)

    -- Draw interactive effects
    draw_particles()

    -- ── HUD Overlay Bar ───────────────────────────────────────
    -- Top overlay sits on empty Row 1
    for i = 1, lives do
        draw_heart(20 + (i - 1) * 12, 6)
    end

    gfx.text("LEVEL " .. current_level, 175, 6, C_BLACK)
    gfx.text("SCORE " .. score, 300, 6, C_BLACK)

    gfx.flip()
end
