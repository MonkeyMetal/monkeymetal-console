-- MonkeyMetal Console — M2 hello world cart
-- Validates: gfx.* binding, input.down(), gfx.flip(), system.time_ms()
-- Controls: hold BOOT key to toggle background color

local bg = gfx.WHITE   -- background color (0=black, 255=white)
local frame = 0

function init()
    system.log("hello world cart init()")
end

function update()
    frame = frame + 1

    -- Toggle background when BOOT key is pressed
    if input.pressed("a") then
        if bg == gfx.WHITE then
            bg = gfx.BLACK
        else
            bg = gfx.WHITE
        end
    end

    -- Exit after 600 frames (~20 seconds) for automated testing
    if frame >= 600 then
        system.log("hello world: auto-exit after 600 frames")
        system.exit()
    end
end

function draw()
    local fg = (bg == gfx.WHITE) and gfx.BLACK or gfx.WHITE

    -- Background
    gfx.clear(bg)

    -- Border
    gfx.rect(4, 4, gfx.W - 8, gfx.H - 8, fg, false)

    -- Centered rectangles (test fill + gray dithering)
    gfx.rect(40, 60, 120, 60, 64, true)   -- dark gray
    gfx.rect(200, 60, 120, 60, 192, true)  -- light gray

    -- Circles
    gfx.circle(100, 200, 35, fg, false)
    gfx.circle(200, 200, 35, 128, true)   -- 50% gray filled
    gfx.circle(300, 200, 35, fg, false)

    -- Diagonal cross
    gfx.line(4, 4, gfx.W - 4, gfx.H - 4, fg)
    gfx.line(gfx.W - 4, 4, 4, gfx.H - 4, fg)

    -- Push to LCD
    gfx.flip()
end
