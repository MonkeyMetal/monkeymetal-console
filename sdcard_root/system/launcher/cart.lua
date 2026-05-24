-- ============================================================
-- MonkeyMetal Console — Launcher (M6)
-- GAMES_LIST → select game → A to launch → back to launcher.
-- All text pure black, highlight = black bg + white text.
-- ============================================================

local W, H = gfx.W, gfx.H
local ON, WH = 0, 255

-- ── State ────────────────────────────────────────────────────
local page = "games"   -- "games" | "menu" | "about"
local games = {}
local sel = 1
local menu_sel = 1
local bt_connected = false

-- ── Load game list from /sdcard/games/ ──────────────────────
-- Folder name is authoritative for display. meta.json (if present) only
-- contributes the version string; no need to author a JSON to add a game,
-- just drop the folder into /sdcard/games/.
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

-- ── Repeat filter (cooldown in frames) ───────────────────────
local cooldown = 0
local function can_repeat()
    if cooldown > 0 then cooldown = cooldown - 1; return false end
    return true
end
local function repeat_done(n) cooldown = n or 8 end

-- ── init ─────────────────────────────────────────────────────
local boot_hold = 0
function init()
    system.log("Launcher init")
    load_games()
    sel = 1
    page = "games"
    cooldown = 0
end

-- ── update ───────────────────────────────────────────────────
function update()
    -- BT status
    bt_connected = input.down("a") or input.down("up") or
                   input.down("left") or input.down("right") or
                   input.down("down")

    if page == "games" then
        if #games > 0 then
            if input.down("up") and can_repeat() then
                sel = sel - 1; if sel < 1 then sel = #games end; repeat_done()
            end
            if input.down("down") and can_repeat() then
                sel = sel + 1; if sel > #games then sel = 1 end; repeat_done()
            end
            if input.down("a") or input.down("system") then
                system.run_cart(games[sel].path)
                return
            end
        end
        if input.down("select") and can_repeat() then
            page = "menu"; menu_sel = 1; repeat_done(15)
        end
        if input.down("y") and can_repeat() then
            page = "about"; repeat_done(15)
        end

    elseif page == "menu" then
        if input.down("up") and can_repeat() then
            menu_sel = menu_sel - 1; if menu_sel < 1 then menu_sel = 4 end; repeat_done()
        end
        if input.down("down") and can_repeat() then
            menu_sel = menu_sel + 1; if menu_sel > 4 then menu_sel = 1 end; repeat_done()
        end
        if input.down("b") or input.down("select") then page = "games"; repeat_done(15) end
        if input.down("a") and menu_sel == 4 then page = "games"; repeat_done(15) end

    elseif page == "about" then
        if input.down("b") or input.down("y") then page = "games"; repeat_done(15) end
    end

    -- BOOT key long-hold fallback
    local boot_now = input.down("system")
    if boot_now then boot_hold = boot_hold + 1 else boot_hold = 0 end
    if boot_hold > 60 then system.exit() end  -- hard exit from launcher
end

-- ── draw ──────────────────────────────────────────────────────
function draw()
    gfx.clear(255)

    -- ＝＝ Top bar ＝＝
    gfx.text("MonkeyMetal", 4, 4, ON)
    -- BT status: filled dot if connected, hollow if not
    if bt_connected then
        gfx.circle(W - 12, 8, 4, ON, true)
    else
        gfx.circle(W - 12, 8, 4, ON, false)
        gfx.circle(W - 12, 8, 3, ON, false)  -- 2px ring
    end
    gfx.text("BT", W - 28, 4, ON)
    gfx.line(0, 18, W, 18, ON)
    gfx.line(0, 19, W, 19, ON)  -- 2px thick

    if page == "games" then
        -- ＝＝ Game list ＝＝
        if #games == 0 then
            gfx.text("No games on SD card.", 8, 120, ON)
            gfx.text("Copy carts to /sdcard/games/", 8, 134, ON)
        else
            local list_y = 24
            local vis = math.floor((H - 56) / 24)  -- visible rows
            -- Scroll to keep selection visible
            local scroll = 0
            if sel > vis then scroll = sel - vis end

            for i = 1, #games do
                local y = list_y + (i - 1 - scroll) * 24
                if y >= list_y and y < H - 32 then
                    if i == sel then
                        -- Highlight: black bg + white text
                        gfx.rect(2, y, W - 4, 22, ON, true)
                        gfx.text("> " .. games[i].name, 8, y + 4, WH)
                        gfx.text(games[i].version, W - 88, y + 4, WH)
                    else
                        gfx.text("  " .. games[i].name, 8, y + 4, ON)
                        gfx.text(games[i].version, W - 88, y + 4, ON)
                    end
                end
            end
        end

        -- ＝＝ Bottom bar ＝＝
        gfx.line(0, H - 18, W, H - 18, ON)
        gfx.line(0, H - 19, W, H - 19, ON)  -- 2px
        gfx.text("UP/DN Sel  A Run  Y Info  SELECT Menu",
                 8, H - 14, ON)

    elseif page == "menu" then
        -- ＝＝ System Menu ＝＝
        local items = {
            "Reconnect Gamepad",
            "Clear Bonded Device",
            "About",
            "Back"
        }
        local my = 40
        for i, label in ipairs(items) do
            local y = my + (i - 1) * 28
            local grey = (i == 2)  -- "Clear Bonded" not implemented yet
            if i == menu_sel then
                gfx.rect(2, y, W - 4, 22, ON, true)
                gfx.text(label, 8, y + 4, WH)
            elseif grey then
                gfx.text(label, 8, y + 4, 48)  -- dimmed
            else
                gfx.text(label, 8, y + 4, ON)
            end
        end

        gfx.line(0, H - 18, W, H - 18, ON)
        gfx.line(0, H - 19, W, H - 19, ON)
        gfx.text("UP/DN Sel  A Enter  B/Back", 8, H - 14, ON)

    elseif page == "about" then
        gfx.text("MonkeyMetal Console", 8, 40, ON)
        gfx.text("M6 Launcher", 8, 54, ON)
        gfx.text("ESP32-S3 + ST7305 1bpp", 8, 68, ON)
        gfx.text("Press Select/B to return", 8, 82, ON)

        gfx.line(0, H - 18, W, H - 18, ON)
        gfx.line(0, H - 19, W, H - 19, ON)
        gfx.text("B Back", 8, H - 14, ON)
    end

    gfx.flip()
end
