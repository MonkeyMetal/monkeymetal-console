-- ============================================================
-- MonkeyMetal Console — Launcher v1.0.1
-- ============================================================

local W, H = gfx.W, gfx.H
local BK, FG = 0, 255
local G1, G2, G3 = 96, 160, 200

-- ── Big digit pixel patterns (3x5 grid, at SCALE) ────────────
local SCALE = 5
local DIGITS = {
    ["0"] = {1,1,1, 1,0,1, 1,0,1, 1,0,1, 1,1,1},
    ["1"] = {0,1,0, 1,1,0, 0,1,0, 0,1,0, 1,1,1},
    ["2"] = {1,1,1, 0,0,1, 1,1,1, 1,0,0, 1,1,1},
    ["3"] = {1,1,1, 0,0,1, 1,1,1, 0,0,1, 1,1,1},
    ["4"] = {1,0,1, 1,0,1, 1,1,1, 0,0,1, 0,0,1},
    ["5"] = {1,1,1, 1,0,0, 1,1,1, 0,0,1, 1,1,1},
    ["6"] = {1,1,1, 1,0,0, 1,1,1, 1,0,1, 1,1,1},
    ["7"] = {1,1,1, 0,0,1, 0,0,1, 0,0,1, 0,0,1},
    ["8"] = {1,1,1, 1,0,1, 1,1,1, 1,0,1, 1,1,1},
    ["9"] = {1,1,1, 1,0,1, 1,1,1, 0,0,1, 1,1,1},
    [":"] = {0,0,0, 0,1,0, 0,0,0, 0,1,0, 0,0,0},
    ["."] = {0,0,0, 0,0,0, 0,0,0, 0,0,0, 1,1,0},
    ["O"] = {1,1,1, 1,0,1, 1,0,1, 1,0,1, 1,1,1},
    ["N"] = {1,0,1, 1,1,1, 1,0,1, 1,0,1, 1,0,1},
    ["F"] = {1,1,1, 1,0,0, 1,1,1, 1,0,0, 1,0,0},
    ["C"] = {1,1,1, 1,0,0, 1,0,0, 1,0,0, 1,1,1},
    ["%"] = {1,0,1, 0,0,1, 0,1,0, 1,0,0, 1,0,1},
}
local CHAR_W = 3 * SCALE + 1
local CHAR_H = 5 * SCALE

local page = "boot" -- Default page is boot splash
local games = {}
local sel = 1
local settings_sel = 1
local volume = 80
local screensaver_timeout = 15
local last_input_ms = 0
local screensaver_active = false
local boot_start_ms = 0

-- ── Starfield system ─────────────────────────────────────
local NUM_STARS = 50
local stars = {}

local function init_stars()
    stars = {}
    for i = 1, NUM_STARS do
        stars[i] = {
            x = math.random(-W, W),
            y = math.random(-H, H),
            z = math.random(1, 256),
        }
    end
end

local function update_stars(speed_override)
    local speed = speed_override or 4
    for i = 1, NUM_STARS do
        local s = stars[i]
        s.z = s.z - speed
        if s.z <= 0 then
            s.x = math.random(-W, W)
            s.y = math.random(-H, H)
            s.z = 256
        end
    end
end

local function draw_stars()
    for i = 1, NUM_STARS do
        local s = stars[i]
        local sx = math.floor((s.x * 200) / s.z + W / 2)
        local sy = math.floor((s.y * 150) / s.z + H / 2)
        if sx >= 0 and sx < W and sy >= 0 and sy < H then
            if s.z < 80 then
                gfx.rect(sx - 1, sy - 1, 2, 2, BK, true)
            else
                gfx.pixel(sx, sy, BK)
            end
        else
            s.x = math.random(-W, W)
            s.y = math.random(-H, H)
            s.z = 256
        end
    end
end

-- ── Fake Boot Logs ───────────────────────────────────────
local BOOT_LOGS = {
    {150,  "[OK] EXT_RAM_BSS INIT"},
    {350,  "[OK] MOUNT SDCARD (/sdcard)"},
    {550,  "[OK] ES8311 I2S CODEC ACTIVE"},
    {750,  "[OK] BLE HOST ON (PAIRING_READY)"},
    {950,  "[OK] LUA 5.4 VM READY"},
    {1150, "[OK] SYSTEM ONLINE!"},
}

-- ── Audio Tracks (Arpeggio state) ────────────────────────
local audio_triggered = {}
local function trigger_boot_audio(elapsed)
    local function play_once(id, freq, dur, vol)
        if not audio_triggered[id] then
            audio.tone(freq, dur, vol)
            audio_triggered[id] = true
        end
    end

    -- Tape / disk seeking ticks
    if elapsed >= 100  and elapsed < 200  then play_once("t1", 1200, 15, 60) end
    if elapsed >= 300  and elapsed < 400  then play_once("t2", 1500, 15, 60) end
    if elapsed >= 600  and elapsed < 700  then play_once("t3", 1000, 15, 60) end
    if elapsed >= 800  and elapsed < 900  then play_once("t4", 1800, 20, 70) end

    -- Rising C Major 7 Arpeggio
    if elapsed >= 1200 and elapsed < 1300 then play_once("c5", 523, 80, 80) end
    if elapsed >= 1300 and elapsed < 1400 then play_once("e5", 659, 80, 80) end
    if elapsed >= 1400 and elapsed < 1500 then play_once("g5", 784, 80, 80) end
    if elapsed >= 1500 and elapsed < 1600 then play_once("b5", 988, 100, 80) end
    if elapsed >= 1600 and elapsed < 2000 then play_once("c6", 1047, 300, 90) end
end

-- ── Load game list ───────────────────────────────────────────
local function load_games()
    games = {}
    local entries = system.list_dir("/sdcard/games")
    local names = {}
    for _, e in ipairs(entries) do
        if e.is_dir then names[#names + 1] = e.name end
    end
    table.sort(names)
    for _, dirname in ipairs(names) do
        local path = "/sdcard/games/" .. dirname
        local version = "-"
        local meta_str = system.read_file(path .. "/meta.json")
        if meta_str then
            local v = string.match(meta_str, '"version"%s*:%s*"([^"]+)"')
            if v then version = v end
        end
        games[#games + 1] = { name = dirname, version = version, path = path }
    end
end

local function any_input()
    return input.pressed("up") or input.pressed("down") or
           input.pressed("left") or input.pressed("right") or
           input.pressed("a") or input.pressed("b") or
           input.pressed("x") or input.pressed("y") or
           input.pressed("start") or input.pressed("select") or
           input.pressed("system")
end

function init()
    system.log("Launcher v1.0.1")
    load_games()
    sel = 1; settings_sel = 1
    page = "boot" -- Default is boot animation
    volume = 80
    audio.master(volume)
    last_input_ms = system.time_ms()
    screensaver_active = false
    boot_start_ms = system.time_ms()
    audio_triggered = {}
    init_stars()
end

function update()
    if page == "boot" then
        local elapsed = system.time_ms() - boot_start_ms
        local speed = 4
        if elapsed < 1200 then
            speed = 24 -- Hyper-drive!
        else
            speed = 3  -- Slow drift
        end
        update_stars(speed)
        trigger_boot_audio(elapsed)

        -- Any key to skip boot animation
        if any_input() and elapsed > 200 then
            page = "games"
            last_input_ms = system.time_ms()
            audio.stop()
        elseif elapsed >= 2800 then
            page = "games"
            last_input_ms = system.time_ms()
        end
        return
    end

    if any_input() then
        last_input_ms = system.time_ms()
        if screensaver_active then screensaver_active = false; page = "games"; return end
    else
        local idle_ms = system.time_ms() - last_input_ms
        if not screensaver_active and idle_ms > screensaver_timeout * 1000 then
            screensaver_active = true; return
        end
    end

    if screensaver_active then
        update_stars(4)
        return
    end

    local up   = input.pressed("up")
    local down = input.pressed("down")
    local left = input.pressed("left")
    local right= input.pressed("right")
    local a    = input.pressed("a")
    local b    = input.pressed("b")

    if page == "games" then
        if up   then sel = sel - 1; if sel < 1 then sel = #games end end
        if down then sel = sel + 1; if sel > #games then sel = 1 end end
        if a and #games > 0 then system.run_cart(games[sel].path); return end
        if input.pressed("select") then page = "settings"; settings_sel = 1 end
        if input.pressed("y") then page = "about" end

    elseif page == "settings" then
        local n = 6
        if up   then settings_sel = settings_sel - 1; if settings_sel < 1 then settings_sel = n end end
        if down then settings_sel = settings_sel + 1; if settings_sel > n then settings_sel = 1 end end
        if settings_sel == 1 and right and volume < 100 then volume = math.min(100, volume+10); audio.master(volume) end
        if settings_sel == 1 and left  and volume > 0   then volume = math.max(0, volume-10); audio.master(volume) end
        if settings_sel == 2 and right then screensaver_timeout = math.min(60, screensaver_timeout+5) end
        if settings_sel == 2 and left  then screensaver_timeout = math.max(5,  screensaver_timeout-5) end
        if b or input.pressed("select") then page = "games" end

    elseif page == "about" then
        if b or input.pressed("y") or input.pressed("select") then page = "games" end
    end

    if input.down("system") then
        boot_hold = (boot_hold or 0) + 1
        if boot_hold > 60 then system.exit() end
    else boot_hold = 0 end
end

-- ═══════════════════════════════════════════════════════════════
--  DRAW
-- ═══════════════════════════════════════════════════════════════
function draw()
    gfx.clear(FG)
    if page == "boot" then draw_boot_page(); gfx.flip(); return end
    if screensaver_active then draw_screensaver(); gfx.flip(); return end

    gfx.text("MonkeyMetal", 4, 4, BK)
    local bt = input.down("a") or input.down("up") or input.down("down")
    gfx.circle(W - 12, 8, 4, BK, bt)
    if not bt then gfx.circle(W - 12, 8, 3, BK, false) end
    gfx.text("BT", W - 28, 4, BK)
    gfx.line(0, 18, W, 18, BK)

    if     page == "games"    then draw_games_page()
    elseif page == "settings" then draw_settings_page()
    elseif page == "about"    then draw_about_page()
    end
    gfx.flip()
end

-- ── Draw Boot Splash Page ───────────────────────────────────
function draw_boot_page()
    local elapsed = system.time_ms() - boot_start_ms

    -- Draw background starfield
    draw_stars()

    if elapsed < 1200 then
        -- Phase 1: Hyper-drive & system checklist
        local log_y = 60
        for _, log in ipairs(BOOT_LOGS) do
            if elapsed >= log[1] then
                gfx.text(log[2], 30, log_y, BK)
                log_y = log_y + 16
            end
        end

        -- Progress bar at bottom
        local bar_w = math.floor((W - 80) * math.min(1.0, elapsed / 1200))
        gfx.rect(40, H - 50, W - 80, 10, BK, false)
        gfx.rect(42, H - 48, bar_w, 6, BK, true)
    else
        -- Phase 2 & 3: Logo display + scanline + ready flash
        local logo_text = "MONKEY METAL"
        local logo_x = math.floor(W / 2 - #logo_text * 4) -- center
        local logo_y = 120

        -- Center logo text
        gfx.text(logo_text, logo_x - 30, logo_y, BK) -- shifted for alignment/aesthetic

        -- Scanline effect
        local scan_elapsed = elapsed - 1200
        if scan_elapsed < 1000 then
            local scan_y = logo_y - 20 + math.floor(scan_elapsed * 0.08)
            if scan_y < logo_y + 30 then
                gfx.line(40, scan_y, W - 40, scan_y, BK)
                -- invert visual dither band
                gfx.rect(40, scan_y - 2, W - 80, 4, BK, false)
            end
        end

        -- SYSTEM READY prompt
        if elapsed >= 2200 then
            local ready_text = "SYSTEM READY!"
            local rx = math.floor(W / 2 - #ready_text * 4)
            -- Blink ready text
            if math.floor(elapsed / 250) % 2 == 0 then
                gfx.text(ready_text, rx, H - 80, BK)
            end
        end
    end
end

-- ═══════════════════════════════════════════════════════════════
--  SCREENSAVER — white BG, high contrast, big layout
-- ═══════════════════════════════════════════════════════════════
function draw_screensaver()
    gfx.clear(FG)

    local ms = system.time_ms()
    local t  = system.time_str() or "--:--:--"
    local ip = system.wifi_ip() or "--"
    local tc = system.temp_c() or 0
    local hu = system.humidity_pct() or 0

    -- ── STARFIELD (3D warp effect) ────────────────────────────
    draw_stars()

    -- ── Foreground panel (white card to block stars behind) ───
    local px = 40
    local pw = W - 80
    local py = 50 + math.floor(math.sin(ms / 3000) * 4)  -- slow float
    local ph = 140
    gfx.rect(px, py, pw, ph, FG, true)    -- pure white fill
    gfx.rect(px, py, pw, ph, BK, false)   -- border
    gfx.rect(px+1, py+1, pw-2, ph-2, BK, false)

    -- ── TIME (big digits, centered) ───────────────────────────
    local time_y = py + 6
    draw_big_time(t, W/2, time_y, ms)

    -- ── Divider ───────────────────────────────────────────────
    local div_y = time_y + CHAR_H + 4
    gfx.line(px + 20, div_y, W - px - 20, div_y, BK)

    -- ── Info rows (inside white panel) ────────────────────────
    local info_x = 120
    local info_y = div_y + 6
    local info_h = 24

    local function info_row(y, label, value_str)
        gfx.text(label, px + 8, y + 4, BK)
        draw_big_str(value_str, info_x, y, 3)  -- smaller scale 3 for info rows
    end

    info_row(info_y,            "TEMP",  string.format("%.1fC", tc))
    info_row(info_y + info_h,   "HUMID", string.format("%.1f%%", hu))
    info_row(info_y + info_h*2, "WIFI",  system.wifi_connected() and "ON" or "OFF")
    gfx.text("IP  " .. ip,     info_x, info_y + info_h*3 + 4, BK)
    -- File server as row inside panel
    local function text_row(y, label, value_str)
        gfx.text(label, px + 8, y + 4, BK)
        gfx.text(value_str, info_x, y + 4, BK)
    end
    text_row(info_y + info_h*4, "FILE", " :9999")

    -- ── Monkey (bottom-right corner, small) ───────────────────
    draw_monkey(W - 60, 240, ms)

    -- ── Wake hint (bottom, pulsing) ───────────────────────────
    local hint = "Press any key"
    local hx = math.floor(W / 2 - #hint * 4)
    if math.floor(ms / 600) % 2 == 0 then
        gfx.text(hint, hx, H - 14, BK)
    end
end

-- Draw a string in big digits (uses SCALE, DIGITS, CHAR_W from top of file)
function draw_big_str(s, x, y, scl)
    for i = 1, #s do
        local ch = s:sub(i, i)
        local p = DIGITS[ch]
        if p then
            local dx = x + (i - 1) * CHAR_W
            for row = 0, 4 do
                for col = 0, 2 do
                    if p[row * 3 + col + 1] == 1 then
                        gfx.rect(dx + col * scl, y + row * scl, scl, scl, BK, true)
                    end
                end
            end
        end
    end
end

function draw_big_time(t_str, cx, yy, ms)
    local total_w = #t_str * CHAR_W
    local x0 = math.floor(cx - total_w / 2)
    draw_big_str(t_str, x0, yy, SCALE)
end

-- Info card not used anymore — replaced by info_row + big digits

-- ── Monkey mascot (cute pixel art) ────────────────────────────
function draw_monkey(cx, y0, ms)
    local bob = math.floor(math.sin(ms / 700) * 2)
    local cy = y0 + bob - 60

    -- Shadow on ground
    gfx.circle(cx, y0 + 2, 22, G1, true)

    -- EARS
    gfx.circle(cx - 26, cy, 14, G1, true)
    gfx.circle(cx - 26, cy, 11, G2, true)
    gfx.circle(cx - 26, cy, 14, BK, false)
    gfx.circle(cx + 26, cy, 14, G1, true)
    gfx.circle(cx + 26, cy, 11, G2, true)
    gfx.circle(cx + 26, cy, 14, BK, false)

    -- HEAD
    gfx.circle(cx, cy, 30, G3, true)
    gfx.circle(cx, cy, 30, BK, false)
    gfx.circle(cx, cy, 29, BK, false)  -- 2px outline

    -- FACE (light patch)
    gfx.circle(cx, cy + 6, 19, G2, true)

    -- EYES
    local ey = cy - 4
    -- Left eye
    gfx.circle(cx - 11, ey, 9, FG, true)
    gfx.circle(cx - 11, ey, 9, BK, false)
    -- Right eye
    gfx.circle(cx + 11, ey, 9, FG, true)
    gfx.circle(cx + 11, ey, 9, BK, false)

    -- Pupils (blink every 3s)
    local blink = math.floor(ms / 3000) % 4 == 0
    if not blink then
        gfx.circle(cx - 9, ey, 4, BK, true)
        gfx.circle(cx + 9, ey, 4, BK, true)
        -- Eye shine
        gfx.pixel(cx - 7, ey - 1, FG)
        gfx.pixel(cx + 11, ey - 1, FG)
    end

    -- NOSE
    gfx.circle(cx, cy + 5, 4, BK, true)

    -- MOUTH
    gfx.line(cx - 8, cy + 12, cx - 4, cy + 16, BK)
    gfx.line(cx - 4, cy + 16, cx + 4, cy + 16, BK)
    gfx.line(cx + 4, cy + 16, cx + 8, cy + 12, BK)

    -- BODY
    gfx.rect(cx - 18, cy + 26, 36, 32, G1, true)
    gfx.rect(cx - 18, cy + 26, 36, 32, BK, false)

    -- ARMS
    local swing = math.floor(math.sin(ms / 450) * 5)
    gfx.rect(cx - 22, cy + 28 + swing, 7, 22, G2, true)
    gfx.rect(cx - 22, cy + 28 + swing, 7, 22, BK, false)
    gfx.rect(cx + 15, cy + 28 - swing, 7, 22, G2, true)
    gfx.rect(cx + 15, cy + 28 - swing, 7, 22, BK, false)

    -- HANDS
    gfx.circle(cx - 18, cy + 48 + swing, 5, G2, true)
    gfx.circle(cx - 18, cy + 48 + swing, 5, BK, false)
    gfx.circle(cx + 19, cy + 48 - swing, 5, G2, true)
    gfx.circle(cx + 19, cy + 48 - swing, 5, BK, false)

    -- LEGS
    gfx.rect(cx - 12, cy + 56, 9, 18, G2, true)
    gfx.rect(cx - 12, cy + 56, 9, 18, BK, false)
    gfx.rect(cx + 3,  cy + 56, 9, 18, G2, true)
    gfx.rect(cx + 3,  cy + 56, 9, 18, BK, false)

    -- FEET
    gfx.rect(cx - 14, cy + 72, 13, 7, BK, true)
    gfx.rect(cx + 1,   cy + 72, 13, 7, BK, true)

    -- TAIL
    local mx = cx + 20
    for i = 0, 6 do
        local tx = mx + i * 5
        local ty = cy + 30 - i * 3 + math.floor(math.sin(i * 0.8) * 7)
        gfx.rect(tx, ty, 4, 4, G1, true)
    end
    -- Tail tip
    gfx.circle(mx + 34, cy + 16, 4, G1, true)

    -- BELLY
    gfx.rect(cx - 9, cy + 36, 18, 14, FG, true)
    gfx.rect(cx - 9, cy + 36, 18, 14, BK, false)
end

-- ═══════════════════════════════════════════════════════════════
--  PAGES
-- ═══════════════════════════════════════════════════════════════

function draw_games_page()
    if #games == 0 then
        gfx.text("No games on SD card.", 8, 120, BK)
        gfx.text("Copy carts to /sdcard/games/", 8, 136, BK)
    else
        local ly = 24
        local vis = math.floor((H - 56) / 24)
        local scroll = 0
        if sel > vis then scroll = sel - vis end
        for i = 1, #games do
            local y2 = ly + (i - 1 - scroll) * 24
            if y2 >= ly and y2 < H - 32 then
                if i == sel then
                    gfx.rect(2, y2, W - 4, 22, BK, true)
                    gfx.text("> " .. games[i].name, 8, y2 + 4, FG)
                    gfx.text(games[i].version, W - 88, y2 + 4, FG)
                else
                    gfx.text("  " .. games[i].name, 8, y2 + 4, BK)
                    gfx.text(games[i].version, W - 88, y2 + 4, BK)
                end
            end
        end
    end
    gfx.line(0, H - 18, W, H - 18, BK)
    gfx.text("UP/DN Sel  A Run  Y Info  SELECT Settings", 8, H - 14, BK)
end

function draw_settings_page()
    gfx.text("SETTINGS", 8, 28, BK)
    gfx.line(8, 42, 160, 42, BK)

    local items = {
        {"Volume",      volume .. "%"},
        {"Screensaver", screensaver_timeout .. "s"},
        {"WiFi",        system.wifi_connected() and "Connected" or "Disconnected"},
        {"IP",          system.wifi_ip() or "0.0.0.0"},
        {"File Server", "port 9999"},
        {"Back",        ""},
    }
    for i, item in ipairs(items) do
        local iy = 50 + (i - 1) * 26
        local is_sel = (settings_sel == i)
        if is_sel then gfx.rect(2, iy - 2, W - 4, 22, BK, true) end
        local c = is_sel and FG or BK
        gfx.text(item[1], 8, iy + 2, c)
        if item[2] ~= "" then gfx.text(item[2], 150, iy + 2, c) end
    end

    gfx.line(0, H - 18, W, H - 18, BK)
    gfx.text("UP/DN Sel  L/R Adjust  B/Back", 8, H - 14, BK)
end

function draw_about_page()
    gfx.text("ABOUT", 8, 28, BK)
    gfx.line(8, 42, 120, 42, BK)
    gfx.text("MonkeyMetal Console", 8, 56, BK)
    gfx.text("v1.3.0  ESP32-S3", 8, 72, BK)
    gfx.text("LCD: ST7305 400x300 1bpp", 8, 88, BK)
    gfx.text("Sensors: SHTC3 temp+humidity", 8, 104, BK)
    gfx.text("RTC: PCF85063", 8, 120, BK)
    gfx.text("Audio: ES8311  Input: BLE Gamepad", 8, 136, BK)
    gfx.text("WiFi + File Server :9999", 8, 152, BK)

    gfx.line(0, H - 18, W, H - 18, BK)
    gfx.text("B/Back", 8, H - 14, BK)
end
