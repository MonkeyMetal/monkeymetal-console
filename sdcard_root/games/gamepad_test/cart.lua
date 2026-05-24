-- ============================================================
-- MonkeyMetal Console — Gamepad Tester
-- ST7305 1bpp, all text pure black (TXT=0), pressed→white (255)
-- Layout: relative W/H, left/right symmetric.
-- ============================================================

local W, H = gfx.W, gfx.H
local ON, OF = 0, 48
local TXT = 0    -- text always readable pure black
local WHT = 255   -- text inside pressed buttons (white on black)

function init() system.log("Gamepad Tester") end

function update()
    _G.hold = (_G.hold or 0)
    if input.pressed("system") then _G.hold = 1
    elseif input.down("system") and _G.hold > 0 then _G.hold = _G.hold + 1
    else _G.hold = 0 end
    if _G.hold > 60 then system.run_cart("/sdcard/system/launcher") end
end

-- ── 2px outline helpers ──────────────────────────────────────
local function r2(x, y, w, h, c)
    gfx.rect(x, y, w, h, c, false)
    gfx.rect(x + 1, y + 1, w - 2, h - 2, c, false)
end
local function c2(cx, cy, r, c)
    gfx.circle(cx, cy, r, c, false)
    gfx.circle(cx, cy, r - 1, c, false)
end

-- ── Button shapes (active = filled black, inactive = 2px outline) ──
local function btn_rect(x, y, w, h, p)
    if p then gfx.rect(x, y, w, h, ON, true) else r2(x, y, w, h, OF) end
end
local function btn_circle(cx, cy, r, p)
    if p then gfx.circle(cx, cy, r, ON, true) else c2(cx, cy, r, OF) end
end

-- ── Text on button: white if pressed, black if not ───────────
local function btn_text(s, x, y, p)
    gfx.text(s, x, y, p and WHT or TXT)
end

-- ── Highlight: black bg + white text ─────────────────────────
local function hl(s, x, y)
    gfx.rect(x - 2, y - 1, #s * 8 + 4, 10, ON, true)
    gfx.text(s, x, y, WHT)
end

function draw()
    gfx.clear(255)

    local a  = input.down("a") or false
    local b  = input.down("b") or false
    local xx = input.down("x") or false
    local yy = input.down("y") or false
    local u  = input.down("up") or false
    local d  = input.down("down") or false
    local l  = input.down("left") or false
    local r  = input.down("right") or false
    local lb = input.down("lb") or false
    local rb = input.down("rb") or false
    local se = input.down("select") or false
    local st = input.down("start") or false
    local lsx = input.ls_x() or 0
    local lsy = input.ls_y() or 0
    local rsx = input.rs_x() or 0
    local rsy = input.rs_y() or 0

    -- Layout constants (all relative)
    local ml = W * 0.06   -- margin left
    local mr = W * 0.94   -- margin right (for right-aligned elements)
    local dpx = W * 0.25  -- D-Pad center X
    local abx = W * 0.75  -- A/B/X/Y center X
    local mid_y = 105     -- vertical center for D-Pad & face buttons
    local st_y = 190      -- stick values start Y

    -- ＝＝ Title ＝＝
    gfx.text("GAMEPAD TEST", W / 2 - 48, 8, TXT)
    gfx.line(16, 22, W - 16, 22, ON)

    -- ＝＝ LB / RB (top row, symmetric) ＝＝
    btn_rect(ml, 32, 72, 22, lb)
    btn_text("LB", ml + 18, 34, lb)
    btn_rect(mr - 72, 32, 72, 22, rb)
    btn_text("RB", mr - 54, 34, rb)

    -- ＝＝ D-Pad cross (left-center) ＝＝
    local sz = 14
    gfx.rect(dpx - sz / 2, mid_y - sz / 2, sz, sz, OF, false)
    btn_rect(dpx - sz / 2, mid_y - sz - sz / 2, sz, sz, u)
    btn_rect(dpx - sz / 2, mid_y + sz / 2, sz, sz, d)
    btn_rect(dpx - sz - sz / 2, mid_y - sz / 2, sz, sz, l)
    btn_rect(dpx + sz / 2, mid_y - sz / 2, sz, sz, r)

    -- D-Pad letters inside buttons
    if u then gfx.text("U", dpx - 3, mid_y - sz - sz / 2 + 2, WHT)
    else   gfx.text("U", dpx - 3, mid_y - sz - sz / 2 + 2, TXT) end
    if d then gfx.text("D", dpx - 3, mid_y + sz / 2 + 3, WHT)
    else   gfx.text("D", dpx - 3, mid_y + sz / 2 + 3, TXT) end
    if l then gfx.text("L", dpx - sz - sz / 2 + 2, mid_y - 3, WHT)
    else   gfx.text("L", dpx - sz - sz / 2 + 2, mid_y - 3, TXT) end
    if r then gfx.text("R", dpx + sz / 2 + 3, mid_y - 3, WHT)
    else   gfx.text("R", dpx + sz / 2 + 3, mid_y - 3, TXT) end

    -- ＝＝ Face buttons (right-center, diamond) ＝＝
    local fr = 14
    btn_circle(abx, mid_y - 26, fr, yy)
    btn_text("Y", abx - 3, mid_y - 36, yy)
    btn_circle(abx - 26, mid_y, fr, xx)
    btn_text("X", abx - 36, mid_y + 3, xx)
    btn_circle(abx + 26, mid_y, fr, b)
    btn_text("B", abx + 20, mid_y + 3, b)
    btn_circle(abx, mid_y + 26, fr, a)
    btn_text("A", abx - 3, mid_y + 38, a)

    -- ＝＝ Stick values (left-aligned, pure black) ＝＝
    gfx.text(string.format("LS: (%+6d,%+6d)", lsx, lsy), ml, st_y, TXT)
    gfx.text(string.format("RS: (%+6d,%+6d)", rsx, rsy), ml, st_y + 14, TXT)

    -- ＝＝ LS / RS click indicators ＝＝
    local cy = st_y + 36
    if se then hl("LS click = SELECT", ml, cy)
    else gfx.text("LS click = SELECT", ml, cy, TXT) end
    if st then hl("RS click = START",  ml, cy + 14)
    else gfx.text("RS click = START",  ml, cy + 14, TXT) end

    -- ＝＝ Status ＝＝
    local any = a or b or xx or yy or u or d or l or r or lb or rb or se or st
    gfx.text(any and "CONNECTED" or "DISCONNECTED", W / 2 - 56, H - 30, TXT)

    -- ＝＝ Exit bar ＝＝
    if _G.hold and _G.hold > 0 then
        gfx.rect(0, H - 6, math.floor((_G.hold / 60) * W), 6, ON, true)
    end

    gfx.flip()
end
