-- ============================================================
-- MonkeyMetal Console — Space Defender  v1.0
-- Controls: D-Pad (left/right) = move ship
--           A / B = shoot
--           START = start/restart
--           SELECT = back to Launcher
-- Rules:    Destroy enemies to score, avoid collisions
--           Different enemies have different points
--           Boss appears every 1000 points
-- Audio:    Complete sound system with BGM, shoot, hit, etc.
-- Screen:   400×300 (gfx.W × gfx.H)
-- ============================================================

-- ── Colors ───────────────────────────────────────────────────
local C_BG      = 255   -- white background
local C_BLACK   = 0     -- black
local C_DARK    = 64    -- dark gray
local C_MID     = 128   -- medium gray
local C_LIGHT   = 192   -- light gray

-- ── Game constants ───────────────────────────────────────────
local SHIP_WIDTH    = 24
local SHIP_HEIGHT   = 16
local BULLET_WIDTH  = 2
local BULLET_HEIGHT = 8
local ENEMY_WIDTH   = 20
local ENEMY_HEIGHT  = 16
local BOSS_WIDTH    = 60
local BOSS_HEIGHT   = 40

-- ── Game state ───────────────────────────────────────────────
local state       -- "title" | "play" | "gameover" | "victory"
local score
local hi_score
local lives
local level
local frame
local blink_timer

-- Player
local player
local bullets = {}

-- Enemies
local enemies = {}
local enemy_bullets = {}
local boss = nil

-- Particles
local particles = {}

-- ── Sound effects ─────────────────────────────────────────────
local function sfx_shoot()
    audio.tone(800, 60, 50)
end

local function sfx_hit()
    audio.tone(400, 80, 60)
end

local function sfx_explode()
    audio.tone(200, 150, 70)
    system.time_ms()
    audio.tone(150, 200, 70)
end

local function sfx_powerup()
    audio.tone(600, 80, 50)
    system.time_ms()
    audio.tone(900, 80, 50)
end

local function sfx_gameover()
    audio.tone(300, 200, 80)
    system.time_ms()
    audio.tone(200, 250, 80)
    system.time_ms()
    audio.tone(100, 300, 80)
end

local function sfx_start()
    audio.tone(523, 100, 60)
    system.time_ms()
    audio.tone(659, 100, 60)
    system.time_ms()
    audio.tone(784, 150, 60)
end

local function bgm_tone()
    -- Simple background music pattern
    local t = frame % 120
    if t == 0 then audio.tone(220, 100, 30)
    elseif t == 30 then audio.tone(262, 100, 30)
    elseif t == 60 then audio.tone(294, 100, 30)
    elseif t == 90 then audio.tone(262, 100, 30)
    end
end

-- ── Helpers ───────────────────────────────────────────────────
local function rand_range(min, max)
    local r = (system.time_ms() * 2654435761) % 10000 / 10000
    return min + r * (max - min)
end

local function aabb_collide(x1, y1, w1, h1, x2, y2, w2, h2)
    return x1 < x2 + w2 and x1 + w1 > x2 and y1 < y2 + h2 and y1 + h1 > y2
end

local function spawn_enemy()
    local type = math.floor(rand_range(1, 4))
    local enemy = {
        x = rand_range(20, gfx.W - 20 - ENEMY_WIDTH),
        y = -ENEMY_HEIGHT,
        width = ENEMY_WIDTH,
        height = ENEMY_HEIGHT,
        type = type,
        health = type,
        speed = 1 + type * 0.5,
        points = type * 10,
        shoot_timer = math.floor(rand_range(30, 90))
    }
    table.insert(enemies, enemy)
end

local function spawn_boss()
    boss = {
        x = gfx.W / 2 - BOSS_WIDTH / 2,
        y = -BOSS_HEIGHT,
        width = BOSS_WIDTH,
        height = BOSS_HEIGHT,
        health = 20,
        max_health = 20,
        speed = 0.8,
        dir = 1,
        shoot_timer = 30
    }
end

local function spawn_particles(x, y, count, color)
    for i = 1, count do
        table.insert(particles, {
            x = x,
            y = y,
            vx = rand_range(-3, 3),
            vy = rand_range(-3, 3),
            life = math.floor(rand_range(20, 40)),
            color = color or C_BLACK
        })
    end
end

local function reset_game()
    player = {
        x = gfx.W / 2 - SHIP_WIDTH / 2,
        y = gfx.H - SHIP_HEIGHT - 20,
        width = SHIP_WIDTH,
        height = SHIP_HEIGHT,
        shoot_cooldown = 0
    }
    bullets = {}
    enemies = {}
    enemy_bullets = {}
    particles = {}
    boss = nil
    score = 0
    lives = 3
    level = 1
    frame = 0
    state = "play"
    blink_timer = 0
end

-- ── init / update / draw ──────────────────────────────────────
function init()
    system.log("Space Defender v1.0 init()")
    audio.master(80)
    hi_score = 0
    state = "title"
    frame = 0
end

function update()
    frame = frame + 1

    -- Title screen
    if state == "title" then
        bgm_tone()
        if input.pressed("a") or input.pressed("start") then
            sfx_start()
            reset_game()
        end
        if input.pressed("select") then
            system.run_cart("/sdcard/system/launcher")
        end
        return
    end

    -- Game over / Victory
    if state == "gameover" or state == "victory" then
        blink_timer = blink_timer + 1
        if input.pressed("a") or input.pressed("start") then
            reset_game()
        end
        if input.pressed("select") then
            system.run_cart("/sdcard/system/launcher")
        end
        return
    end

    -- Playing
    bgm_tone()

    -- Player movement
    if input.down("left") and player.x > 5 then
        player.x = player.x - 4
    end
    if input.down("right") and player.x < gfx.W - player.width - 5 then
        player.x = player.x + 4
    end

    -- Player shooting
    if player.shoot_cooldown > 0 then
        player.shoot_cooldown = player.shoot_cooldown - 1
    end
    if (input.pressed("a") or input.pressed("b")) and player.shoot_cooldown == 0 then
        sfx_shoot()
        table.insert(bullets, {
            x = player.x + player.width / 2 - BULLET_WIDTH / 2,
            y = player.y - BULLET_HEIGHT,
            width = BULLET_WIDTH,
            height = BULLET_HEIGHT
        })
        player.shoot_cooldown = 10
    end

    -- Update bullets
    for i = #bullets, 1, -1 do
        bullets[i].y = bullets[i].y - 6
        if bullets[i].y < -BULLET_HEIGHT then
            table.remove(bullets, i)
        end
    end

    -- Update enemy bullets
    for i = #enemy_bullets, 1, -1 do
        enemy_bullets[i].y = enemy_bullets[i].y + 5
        if enemy_bullets[i].y > gfx.H then
            table.remove(enemy_bullets, i)
        end
    end

    -- Spawn enemies
    if not boss and #enemies < 5 + level and frame % (60 - level * 5) == 0 then
        spawn_enemy()
    end

    -- Spawn boss
    if not boss and score >= level * 1000 then
        spawn_boss()
        sfx_powerup()
    end

    -- Update enemies
    for i = #enemies, 1, -1 do
        local e = enemies[i]
        e.y = e.y + e.speed

        -- Enemy shooting
        e.shoot_timer = e.shoot_timer - 1
        if e.shoot_timer <= 0 then
            table.insert(enemy_bullets, {
                x = e.x + e.width / 2 - 1,
                y = e.y + e.height,
                width = 2,
                height = 6
            })
            e.shoot_timer = math.floor(rand_range(40, 100))
        end

        -- Remove off-screen enemies
        if e.y > gfx.H then
            table.remove(enemies, i)
        end
    end

    -- Update boss
    if boss then
        if boss.y < 20 then
            boss.y = boss.y + boss.speed
        else
            boss.x = boss.x + boss.dir * boss.speed
            if boss.x <= 20 or boss.x + boss.width >= gfx.W - 20 then
                boss.dir = -boss.dir
            end

            -- Boss shooting
            boss.shoot_timer = boss.shoot_timer - 1
            if boss.shoot_timer <= 0 then
                for i = -1, 1 do
                    table.insert(enemy_bullets, {
                        x = boss.x + boss.width / 2 - 1 + i * 15,
                        y = boss.y + boss.height,
                        width = 3,
                        height = 8
                    })
                end
                boss.shoot_timer = 40
            end
        end
    end

    -- Update particles
    for i = #particles, 1, -1 do
        local p = particles[i]
        p.x = p.x + p.vx
        p.y = p.y + p.vy
        p.life = p.life - 1
        if p.life <= 0 then
            table.remove(particles, i)
        end
    end

    -- Bullet vs Enemy collisions
    for i = #bullets, 1, -1 do
        local b = bullets[i]
        for j = #enemies, 1, -1 do
            local e = enemies[j]
            if aabb_collide(b.x, b.y, b.width, b.height, e.x, e.y, e.width, e.height) then
                e.health = e.health - 1
                table.remove(bullets, i)
                if e.health <= 0 then
                    score = score + e.points
                    if score > hi_score then hi_score = score end
                    spawn_particles(e.x + e.width / 2, e.y + e.height / 2, 8)
                    sfx_explode()
                    table.remove(enemies, j)
                else
                    sfx_hit()
                end
                break
            end
        end

        -- Bullet vs Boss collision
        if boss and bullets[i] then
            if aabb_collide(b.x, b.y, b.width, b.height, boss.x, boss.y, boss.width, boss.height) then
                boss.health = boss.health - 1
                table.remove(bullets, i)
                if boss.health <= 0 then
                    score = score + 500
                    if score > hi_score then hi_score = score end
                    spawn_particles(boss.x + boss.width / 2, boss.y + boss.height / 2, 20)
                    sfx_explode()
                    boss = nil
                    level = level + 1
                    if level > 5 then
                        state = "victory"
                        sfx_start()
                    end
                else
                    sfx_hit()
                end
            end
        end
    end

    -- Enemy bullet vs Player collision
    for i = #enemy_bullets, 1, -1 do
        local eb = enemy_bullets[i]
        if aabb_collide(eb.x, eb.y, eb.width, eb.height, player.x, player.y, player.width, player.height) then
            table.remove(enemy_bullets, i)
            lives = lives - 1
            spawn_particles(player.x + player.width / 2, player.y + player.height / 2, 10)
            sfx_explode()
            if lives <= 0 then
                state = "gameover"
                sfx_gameover()
            end
        end
    end

    -- Enemy vs Player collision
    for i = #enemies, 1, -1 do
        local e = enemies[i]
        if aabb_collide(e.x, e.y, e.width, e.height, player.x, player.y, player.width, player.height) then
            table.remove(enemies, i)
            lives = lives - 1
            spawn_particles(player.x + player.width / 2, player.y + player.height / 2, 12)
            sfx_explode()
            if lives <= 0 then
                state = "gameover"
                sfx_gameover()
            end
        end
    end

    -- Select to exit
    if input.pressed("select") then
        system.run_cart("/sdcard/system/launcher")
    end
end

-- ── Draw helpers ─────────────────────────────────────────────
local function draw_ship(x, y)
    -- Main body
    gfx.rect(x + 8, y, 8, 16, C_BLACK, true)
    -- Wings
    gfx.rect(x, y + 8, 24, 8, C_BLACK, true)
    -- Cockpit
    gfx.rect(x + 10, y + 2, 4, 4, C_BG, true)
    -- Engine glow
    gfx.rect(x + 10, y + 14, 4, 4, C_LIGHT, true)
end

local function draw_enemy(e)
    local x = math.floor(e.x)
    local y = math.floor(e.y)
    if e.type == 1 then
        gfx.rect(x + 4, y, 12, 12, C_BLACK, true)
        gfx.rect(x, y + 4, 20, 8, C_BLACK, true)
    elseif e.type == 2 then
        gfx.rect(x, y, 20, 16, C_BLACK, true)
        gfx.rect(x + 2, y + 2, 16, 12, C_BG, true)
        gfx.rect(x + 4, y + 4, 12, 8, C_BLACK, true)
    else
        gfx.rect(x, y, 20, 16, C_BLACK, true)
        gfx.rect(x + 4, y, 12, 4, C_BG, true)
        gfx.rect(x, y + 8, 20, 4, C_BG, true)
    end
end

local function draw_boss(b)
    local x = math.floor(b.x)
    local y = math.floor(b.y)
    gfx.rect(x, y, b.width, b.height, C_BLACK, true)
    gfx.rect(x + 4, y + 4, b.width - 8, b.height - 8, C_BG, true)
    gfx.rect(x + 8, y + 8, b.width - 16, b.height - 16, C_BLACK, true)
    -- Eyes
    gfx.rect(x + 12, y + 12, 8, 8, C_BG, true)
    gfx.rect(x + b.width - 20, y + 12, 8, 8, C_BG, true)
    -- Health bar
    local bar_width = (b.health / b.max_health) * 50
    gfx.rect(x + 5, y - 10, 50, 6, C_BLACK, false)
    gfx.rect(x + 6, y - 9, bar_width, 4, C_BLACK, true)
end

function draw()
    gfx.clear(C_BG)

    if state == "title" then
        -- Title
        gfx.rect(80, 60, 240, 180, C_BLACK, false)
        gfx.rect(84, 64, 232, 172, C_BLACK, false)

        -- Spaceship
        draw_ship(188, 90)

        -- Title bars
        gfx.rect(100, 140, 200, 20, C_BLACK, true)
        gfx.rect(104, 144, 192, 12, C_BG, true)

        -- Start hint
        for i = 0, 10 do
            gfx.rect(110 + i * 16, 180, 12, 6, C_BLACK, true)
        end

        -- Hi-score bar
        if hi_score > 0 then
            local bar = math.min(hi_score / 20, 200)
            gfx.rect(100, 210, bar, 8, C_DARK, true)
            gfx.rect(100, 210, 200, 8, C_BLACK, false)
        end

        gfx.flip()
        return
    end

    -- Play field
    -- Stars
    for i = 0, 19 do
        local sx = (i * 73 + frame) % gfx.W
        local sy = (i * 47 + frame * 2) % gfx.H
        gfx.pixel(sx, sy, C_LIGHT)
    end

    -- Draw player
    draw_ship(player.x, player.y)

    -- Draw bullets
    for _, b in ipairs(bullets) do
        local bx = math.floor(b.x)
        local by = math.floor(b.y)
        gfx.rect(bx, by, b.width, b.height, C_BLACK, true)
    end

    -- Draw enemy bullets
    for _, eb in ipairs(enemy_bullets) do
        local ex = math.floor(eb.x)
        local ey = math.floor(eb.y)
        gfx.rect(ex, ey, eb.width, eb.height, C_BLACK, true)
    end

    -- Draw enemies
    for _, e in ipairs(enemies) do
        draw_enemy(e)
    end

    -- Draw boss
    if boss then
        draw_boss(boss)
    end

    -- Draw particles
    for _, p in ipairs(particles) do
        local size = math.floor(p.life / 10) + 1
        local px = math.floor(p.x - size / 2)
        local py = math.floor(p.y - size / 2)
        gfx.rect(px, py, size, size, p.color, true)
    end

    -- UI
    -- Lives
    for i = 0, lives - 1 do
        gfx.rect(10 + i * 30, 10, 20, 12, C_BLACK, false)
    end

    -- Score
    local bar_w = math.floor(math.min(score / 20, 180))
    gfx.rect(gfx.W - 200, 10, 190, 12, C_BLACK, false)
    gfx.rect(gfx.W - 198, 12, bar_w, 8, C_DARK, true)

    -- Level
    gfx.rect(gfx.W / 2 - 25, 10, 50, 12, C_BLACK, false)

    -- Game over / Victory overlay
    if state == "gameover" or state == "victory" then
        local blink = math.floor(blink_timer / 15) % 2 == 0
        if blink then
            gfx.rect(50, 80, 300, 140, C_BLACK, false)
        end
        gfx.rect(60, 90, 280, 120, C_BG, true)
        gfx.rect(60, 90, 280, 120, C_BLACK, false)

        -- Message
        local msg = state == "gameover" and "GAME OVER" or "VICTORY!"
        gfx.rect(100, 110, 200, 20, C_BLACK, true)
        gfx.rect(104, 114, 192, 12, C_BG, true)

        -- Score bar
        local bar = math.floor(math.min(score / 30, 200))
        gfx.rect(100, 150, bar, 12, C_BLACK, true)
        gfx.rect(100, 150, 200, 12, C_BLACK, false)

        -- Restart hint
        for i = 0, 7 do
            gfx.rect(120 + i * 20, 180, 14, 6, C_BLACK, true)
        end
    end

    gfx.flip()
end
