-- Layered mascot animation for Mascot.ini. All animation state lives here.
-- Rig: the lower body stays still; the torso leans around the waist; the head turns around the neck; the
-- back hair swings around the neck; the ahoge and the two whale fins swing around their roots and are carried
-- by the head; the tail swings on the lower body. The irises follow the mouse cursor (MeasureCursor) inside the whites
-- of the eyes; while the ToDo window is shown they rest on it unless the cursor is over it or near her.
-- Motion: every angle is a damped spring chasing a target, so parts lag, overshoot, and settle; the hair and
-- the ahoge are pushed by the head's movement; idle targets mix incommensurate waves with occasional events
-- (weight shifts, head tilts, tail flicks), so the loop never repeats exactly. Dragging the skin pushes every
-- spring opposite to the window's acceleration, so she sways when picked up and swings after being let go,
-- and while she is carried her hair, ahoge, and tail trail behind.
-- Sleep: after SLEEP.after seconds without keyboard or mouse input, or while a full-screen app is in front
-- (MeasureDesktop), she dozes off: the eyes close and the head droops; once settled she holds still and the
-- script only checks whether to wake her. Input, a drag, or an action wakes her with a blink and a fin flick.
-- Meters never update on their own (DefaultUpdateDivider=-1 in Mascot.ini): !ShowMeter, !HideMeter, and
-- !MoveMeter take effect at once, and a meter changed by !SetOption is updated explicitly with !UpdateMeter.
-- Idle: every 30 to 60 seconds (uniformly sampled) she eats (weight 4), smiles (3), or writes a short note
-- (3, four seconds) on her own. While the ToDo window is shown and she is awake, writing in her notebook is
-- her resting pose: eating and the happy face still play over it (the short note does not), she still
-- blinks and sways, and every 6 to 14 seconds she looks up from the notebook for a moment.
-- Pat(): happy face, head tilt, ahoge bounce, and a hop. Eat(): bowl and spoon, open mouth, then chewing with
-- squinting eyes and the head tilted. StartWriting() / StopWriting(): notebook pose with eyes down while Main
-- makes a plan. ToggleTodo(): show or hide the ToDo\Main window.
-- ClickTail() / ClickFin(side): a flick of that part, and the chat bubble opens or closes (ToggleCompanion()).
-- ClickHead() / ClickBody(): a click on her (Mascot.ini) pats or feeds her only after clicks.wait seconds,
-- because the first click of a double-click (ToggleTodo) also arrives as a click; ToggleTodo() drops a
-- waiting click and ignores the clicks that follow within clicks.wait.
-- Frames are prepared by design/mascot/export_skin_assets.py; here they are only shown, hidden, and moved.

local TAU = 2 * math.pi
-- Constants and state are grouped into tables: Lua 5.1 allows a function at most 60 upvalues, and Update()
-- reads most of them.
local ACTIONS = {
    -- Played on her own while idle, picked by weight: eating a little more often than the others.
    idle = { { name = 'eat', weight = 4 }, { name = 'pat', weight = 3 }, { name = 'write', weight = 3 } },
    idle_min = 30, idle_max = 60, -- seconds between them, sampled uniformly
    eat_seconds = 3.2,
    pat_seconds = 1.6,
    write_seconds = 4.0,          -- a spontaneous note; while Main makes a plan she writes until it is done
    eat_tilt = 10.0,              -- degrees of head tilt while chewing
    pat_tilt = 3.0,
}
local GAZE = {
    x = 3, y = 2,                 -- largest iris shift, skin pixels
    reach = 220,                  -- cursor distance (skin pixels) at which the shift is full
    rate = 21,                    -- per second: the iris covers 1 - exp(-rate * dt) of the remaining distance
}
-- Dragging: spring velocity added per skin pixel per second of change in the window's velocity.
local DRAG = {
    lean = 0.012, head = 0.015, hair = 0.06, ahoge = 0.08, tail = 0.05,
    fin = 0.08,                   -- vertical moves only: the fins point sideways
    speed_max = 4000,             -- skin pixels per second; faster is a jump (a refresh or !Move), not a drag
}
-- Carried: spring target offset per skin pixel per second of the window's velocity (degrees, or source
-- pixels for the hair tips), up to TRAIL.speed_max.
local TRAIL = {
    lean = 0.0015, head = 0.004, hair = 0.03, ahoge = 0.012, tail = 0.008,
    fin = 0.006,                  -- vertical moves only
    speed_max = 1500,
}
local SLEEP = {
    after = 90,                   -- seconds without keyboard or mouse input
    tilt = 6.0,                   -- degrees the head droops while asleep
    settle = 4.0,                 -- seconds after dozing off until she holds still
}

local frame        -- seconds since the previous update, measured by MeasureCursor's clock
local clock_ms     -- that clock's previous reading
local t
local todo_shown   -- whether ToDo\Main is shown; it starts shown
local angles       -- name -> { first, step, count }
-- Pivots in skin pixels: eye_x/y (gaze origin), neck_x/y (the head turns around it), chest_x/y (the torso
-- leans around it), root_x/y (the ahoge's root), fin_roots ('finl' / 'finr' -> { x, y }).
local rig
local FIN_UP = { finl = 1, finr = -1 } -- rotation sign that lifts each fin's tip (the left fin points left)
local home         -- meter -> { x, y } as placed by the skin
local shown        -- meter -> true for the frames on screen
local matrixed     -- meter -> true while it carries a TransformationMatrix
local offsets      -- part -> { dx, dy } currently applied
local lean, head, hair, ahoge, tail -- springs { x, v }
local fins         -- 'finl' / 'finr' -> spring
local events       -- idle event state
local iris         -- { x, y }: the current iris shift, easing toward the gaze target
local blink        -- { next = time of the next blink, start = time the current one started or nil }
local action       -- nil | 'eat' | 'pat'
local action_start
-- A click waiting to become a pat or a meal: { name = nil | 'pat' | 'eat', at, ignored_until }. wait is
-- Windows' default double-click time, 0.5 s, less a margin for the update interval.
local clicks
local writing
-- The skin's window: x/y at the previous update, origin_x/y now (screen pixels, #CURRENTCONFIGX/Y#), and
-- vx/vy, its smoothed velocity in skin pixels per second.
local window
local panel        -- the ToDo window: { visible, left, top, right, bottom } in screen pixels
local idle_seconds -- since the last keyboard or mouse input
local fullscreen   -- a full-screen app is in front
local sleep_state  -- { asleep, start = when she dozed off }

local function variable(name)
    return tonumber(SKIN:GetVariable(name)) or 0
end

local function angle_set(name)
    return { first = variable(name .. 'First'), step = variable(name .. 'Step'), count = variable(name .. 'Frames') }
end

-- Nearest frame index for an angle, clamped to the prepared range.
local function index(set, degrees)
    local i = math.floor((degrees - set.first) / set.step + 0.5)
    return math.max(0, math.min(set.count - 1, i))
end

local function smoothstep(a, b, x)
    local u = math.max(0, math.min(1, (x - a) / (b - a)))
    return u * u * (3 - 2 * u)
end

local function round(v)
    return math.floor(v + 0.5)
end

local function random_between(a, b)
    return a + (b - a) * math.random()
end

-- Damped spring: frequency in Hz, damping ratio (1 = no overshoot).
local function spring(s, target, frequency, damping)
    local w = TAU * frequency
    local accel = -w * w * (s.x - target) - 2 * damping * w * s.v
    s.v = s.v + accel * frame
    s.x = s.x + s.v * frame
end

function Initialize()
    rig, iris, blink, window, sleep_state = {}, {}, {}, {}, {}
    frame = variable('MascotFrameMs') / 1000
    clock_ms = nil
    t = 0
    todo_shown = true
    angles = {
        lean = angle_set('Lean'), head = angle_set('Head'), hair = angle_set('Hair'),
        ahoge = angle_set('Ahoge'), tail = angle_set('Tail'), fin = angle_set('Fin'),
    }
    rig.eye_x, rig.eye_y = variable('EyeCenterX'), variable('EyeCenterY')
    rig.neck_x, rig.neck_y = variable('NeckX'), variable('NeckY')
    rig.chest_x, rig.chest_y = variable('ChestX'), variable('ChestY')
    rig.root_x, rig.root_y = variable('AhogeRootX'), variable('AhogeRootY')
    rig.fin_roots = {
        finl = { variable('FinLRootX'), variable('FinLRootY') },
        finr = { variable('FinRRootX'), variable('FinRRootY') },
    }
    home = {}
    shown = {}
    matrixed = {}
    offsets = { body = { 0, 0 }, head = { 0, 0 }, ahoge = { 0, 0 }, iris = { 0, 0 }, finl = { 0, 0 }, finr = { 0, 0 } }
    lean, head, hair, ahoge, tail = { x = 0, v = 0 }, { x = 0, v = 0 }, { x = 0, v = 0 }, { x = 0, v = 0 }, { x = 0, v = 0 }
    fins = { finl = { x = 0, v = 0 }, finr = { x = 0, v = 0 } } -- flap: positive lifts the tip
    math.randomseed(os.time())
    events = {
        weight = 0, next_weight = random_between(6, 12),
        tilt = 0, tilt_until = 0, next_tilt = random_between(5, 10),
        next_flick = random_between(4, 8),
        next_twitch = random_between(3, 8),
        next_lookup = random_between(6, 14), lookup_until = 0,
        next_action = random_between(ACTIONS.idle_min, ACTIONS.idle_max),
    }
    iris.x, iris.y = 0, 0
    blink.next = 1.5 + math.random() * 2
    blink.start = nil
    action = nil
    -- bubble_at: when the tail or a fin last opened or closed the chat bubble.
    clicks = { name = nil, at = 0, ignored_until = 0, wait = 0.45, bubble_at = -1 }
    writing = false
    window.x, window.y = nil, nil
    window.origin_x, window.origin_y = nil, nil
    window.vx, window.vy = 0, 0
    panel = { visible = false }
    idle_seconds = 0
    fullscreen = false
    sleep_state.asleep = false
    sleep_state.start = 0
end

-- ------------------------------------------------------------ frames on screen

-- Layers that ride on the head (besides the ahoge, fins, and the contained iris).
local HEAD_PREFIXES = {
    'Head', 'Sclera', 'IrisClip', 'Lashes', 'Hair', 'EyesHalf', 'EyesClosed', 'EyesDown', 'EyesHappy',
    'MouthOpen', 'MouthChew', 'FaceHappy',
}

local function part_of(meter)
    for _, prefix in ipairs(HEAD_PREFIXES) do
        if meter:sub(1, #prefix) == prefix then
            return 'head'
        end
    end
    if meter:sub(1, 5) == 'Ahoge' then
        return 'ahoge'
    elseif meter:sub(1, 4) == 'FinL' then
        return 'finl'
    elseif meter:sub(1, 4) == 'FinR' then
        return 'finr'
    elseif meter:sub(1, 4) == 'Iris' and meter:sub(1, 8) ~= 'IrisClip' then
        return 'iris'
    end
    return 'body'
end

-- Parts carried by the turning head (the ahoge and the fins) follow it by a fractional offset: whole-pixel
-- steps against the head's smooth frames read as stutter. The head layers move in whole pixels (only while
-- she rocks after a pat): a TransformationMatrix on them costs a tenth of a CPU core, and clearing it does not
-- take a meter off Rainmeter's slow drawing path again.
local SUBPIXEL = { ahoge = true, finl = true, finr = true }

local function place(meter)
    local part = part_of(meter)
    local o = offsets[part]
    if SUBPIXEL[part] then
        -- Any TransformationMatrix, even an empty one, puts the meter on Rainmeter's slow drawing path, so a
        -- meter with no offset is left alone unless it carried one before.
        if o[1] ~= 0 or o[2] ~= 0 then
            SKIN:Bang('!SetOption', meter, 'TransformationMatrix', string.format('1;0;0;1;%.2f;%.2f', o[1], o[2]))
            SKIN:Bang('!UpdateMeter', meter)
            matrixed[meter] = true
        elseif matrixed[meter] then
            SKIN:Bang('!SetOption', meter, 'TransformationMatrix', '')
            SKIN:Bang('!UpdateMeter', meter)
            matrixed[meter] = nil
        end
        return
    end
    if part == 'iris' then
        -- The iris sits in its clip container (which already moves with the hop), at a position relative to
        -- it; GetX() reports contained meters shifted twice, so the rest position comes from RestX / RestY.
        if not home[meter] then
            local m = SKIN:GetMeter(meter)
            home[meter] = { tonumber(m:GetOption('RestX')), tonumber(m:GetOption('RestY')) }
        end
        SKIN:Bang('!SetOption', meter, 'X', home[meter][1] + o[1])
        SKIN:Bang('!SetOption', meter, 'Y', home[meter][2] + o[2])
        SKIN:Bang('!UpdateMeter', meter)
        return
    end
    if not home[meter] then
        local m = SKIN:GetMeter(meter)
        home[meter] = { m:GetX(), m:GetY() }
    end
    SKIN:Bang('!MoveMeter', home[meter][1] + o[1], home[meter][2] + o[2], meter)
end

-- Shows exactly the meters in `wanted` (a set); hides the rest of the previous frame; re-places the
-- meters of every part whose offset changed.
local function present(wanted, moved)
    for meter in pairs(wanted) do
        if not shown[meter] or moved[part_of(meter)] then
            place(meter)
        end
        if not shown[meter] then
            SKIN:Bang('!ShowMeter', meter)
        end
    end
    for meter in pairs(shown) do
        if not wanted[meter] then
            SKIN:Bang('!HideMeter', meter)
        end
    end
    shown = wanted
end

local function set_offset(part, dx, dy, moved)
    local o = offsets[part]
    if math.abs(o[1] - dx) >= 0.02 or math.abs(o[2] - dy) >= 0.02 then
        offsets[part] = { dx, dy }
        moved[part] = true
    end
end

function Start()
    -- The skin starts with the rest frame of every part shown (Hidden=0 in frames.inc).
    local h = index(angles.head, 0)
    shown = {
        ['Hair' .. index(angles.hair, 0)] = true,
        ['Tail' .. index(angles.tail, 0)] = true,
        ['Ahoge' .. index(angles.ahoge, 0)] = true,
        ['FinL' .. index(angles.fin, 0)] = true,
        ['FinR' .. index(angles.fin, 0)] = true,
        LowerIdle = true,
        ['UpperIdle' .. index(angles.lean, 0)] = true,
        ['Head' .. h] = true, ['Sclera' .. h] = true, ['IrisClip' .. h] = true, ['Iris' .. h] = true,
        ['Lashes' .. h] = true,
    }
end

-- ------------------------------------------------------------ inputs

-- Reads MeasureCursor ("X,Y,MS"): advances the frame time by the real elapsed time (the Windows timer
-- fires on 15.6 ms ticks, so Update is only nominal) and returns the cursor position in screen pixels.
local function read_cursor()
    local text = SKIN:GetMeasure('MeasureCursor'):GetStringValue()
    local cx, cy, ms = text:match('^(-?%d+),(-?%d+),([%d%.]+)$')
    if not cx then
        return nil, nil
    end
    ms = tonumber(ms)
    if clock_ms then
        -- Clamp: a stalled desktop must not make the springs jump.
        frame = math.max(0.004, math.min(0.05, (ms - clock_ms) / 1000))
    end
    clock_ms = ms
    return tonumber(cx), tonumber(cy)
end

-- Parses a DesktopState measure ("VISIBLE,LEFT,TOP,RIGHT,BOTTOM,IDLE_MS,FULLSCREEN", plugins/desktop_state).
local function desktop_state(measure)
    local text = SKIN:GetMeasure(measure):GetStringValue()
    local v, l, tp, r, b, idle, full = text:match('^(%d),(-?%d+),(-?%d+),(-?%d+),(-?%d+),(%d+),(%d)$')
    if not v then
        return nil
    end
    return {
        visible = v == '1', left = tonumber(l), top = tonumber(tp), right = tonumber(r), bottom = tonumber(b),
        idle_seconds = tonumber(idle) / 1000, fullscreen = full == '1',
    }
end

-- Reads MeasureDesktop (the ToDo window, idle time, full-screen apps) and the skin's position, which
-- changes while the skin is dragged.
local function read_desktop()
    local desktop = desktop_state('MeasureDesktop')
    if desktop then
        panel = desktop
        idle_seconds = desktop.idle_seconds
        fullscreen = desktop.fullscreen
    end
    window.origin_x, window.origin_y = variable('CURRENTCONFIGX'), variable('CURRENTCONFIGY')
end

-- The iris shift toward a screen point: full at GAZE.reach skin pixels from her eyes.
local function gaze_toward(x, y)
    local vx = x - (window.origin_x + rig.eye_x)
    local vy = y - (window.origin_y + rig.eye_y)
    local distance = math.sqrt(vx * vx + vy * vy)
    if distance <= 1 then
        return 0, 0
    end
    local reach = math.min(1, distance / GAZE.reach)
    return GAZE.x * reach * vx / distance, GAZE.y * reach * vy / distance
end

-- She watches the cursor; while the ToDo window is shown she looks at it instead, unless the cursor is over
-- the window or within GAZE.reach of her eyes.
local function gaze_target(cx, cy)
    if not cx or not window.origin_x then
        return 0, 0
    end
    if panel.visible then
        local over = cx >= panel.left and cx < panel.right and cy >= panel.top and cy < panel.bottom
        local dx = cx - (window.origin_x + rig.eye_x)
        local dy = cy - (window.origin_y + rig.eye_y)
        if not over and dx * dx + dy * dy > GAZE.reach * GAZE.reach then
            return gaze_toward((panel.left + panel.right) / 2, (panel.top + panel.bottom) / 2)
        end
    end
    return gaze_toward(cx, cy)
end

-- The window's change in velocity since the previous update (skin pixels per second).
local function window_push()
    if not window.origin_x then
        return 0, 0
    end
    local x, y = window.origin_x, window.origin_y
    if not window.x then
        window.x, window.y = x, y
        return 0, 0
    end
    local vx, vy = (x - window.x) / frame, (y - window.y) / frame
    window.x, window.y = x, y
    if math.abs(vx) > DRAG.speed_max or math.abs(vy) > DRAG.speed_max then
        vx, vy = 0, 0
    end
    -- Mouse moves arrive unevenly; smoothing the velocity over a few updates keeps the push from flickering.
    local ease = 1 - math.exp(-25 * frame)
    local nvx = window.vx + (vx - window.vx) * ease
    local nvy = window.vy + (vy - window.vy) * ease
    local dvx, dvy = nvx - window.vx, nvy - window.vy
    window.vx, window.vy = nvx, nvy
    return dvx, dvy
end

-- Picks an idle action by weight, leaving out `skip` (nil: none).
local function pick_idle_action(skip)
    local total = 0
    for _, a in ipairs(ACTIONS.idle) do
        if a.name ~= skip then
            total = total + a.weight
        end
    end
    local roll = math.random() * total
    for _, a in ipairs(ACTIONS.idle) do
        if a.name ~= skip then
            roll = roll - a.weight
            if roll < 0 then
                return a.name
            end
        end
    end
    return ACTIONS.idle[1].name
end

local function start_action(name)
    action, action_start = name, t
    -- Any action, asked for or spontaneous, restarts the wait for the next spontaneous one.
    events.next_action = t + random_between(ACTIONS.idle_min, ACTIONS.idle_max)
end

-- Occasional idle events: a weight shift, a curious head tilt, a tail flick, and every 30 to 60 seconds one
-- of the actions (eating, the happy pat face, a short note in the notebook) when nothing else is playing.
local function idle_events()
    if t >= events.next_action then
        if action == nil and not writing then
            -- At the notebook already, a short note would look like nothing happened.
            start_action(pick_idle_action(panel.visible and 'write' or nil))
        else
            events.next_action = t + random_between(ACTIONS.idle_min, ACTIONS.idle_max)
        end
    end
    if t >= events.next_weight then
        events.weight = random_between(-0.6, 0.6)
        events.next_weight = t + random_between(6, 14)
    end
    if t >= events.next_tilt then
        events.tilt = (math.random() < 0.5 and -1 or 1) * random_between(2.0, 4.0)
        events.tilt_until = t + random_between(1.0, 2.2)
        events.next_tilt = t + random_between(7, 15)
    end
    if t >= events.tilt_until then
        events.tilt = 0
    end
    if t >= events.next_twitch then
        -- One fin, sometimes both, flicks up and springs back.
        local which = math.random(3)
        if which ~= 2 then fins.finl.v = fins.finl.v + random_between(110, 190) end
        if which ~= 1 then fins.finr.v = fins.finr.v + random_between(110, 190) end
        events.next_twitch = t + random_between(3, 8)
    end
    if t >= events.next_lookup then
        events.lookup_until = t + random_between(1.2, 2.5)
        events.next_lookup = events.lookup_until + random_between(6, 14)
    end
    if t >= events.next_flick then
        tail.v = tail.v + (math.random() < 0.5 and -1 or 1) * random_between(60, 110)
        events.next_flick = t + random_between(4, 10)
    end
end

-- Dozes off after SLEEP.after seconds without input or behind a full-screen app; wakes on input, a drag,
-- or an action.
local function update_sleep(dragged)
    local drowsy = (idle_seconds >= SLEEP.after or fullscreen) and action == nil and not writing and not dragged
    if drowsy and not sleep_state.asleep then
        sleep_state.asleep, sleep_state.start = true, t
    elseif not drowsy and sleep_state.asleep then
        sleep_state.asleep = false
        -- Waking up: a blink and a flick of both fins.
        blink.start = t
        fins.finl.v = fins.finl.v + 150
        fins.finr.v = fins.finr.v + 150
        events.next_action = t + random_between(ACTIONS.idle_min, ACTIONS.idle_max)
    end
end

-- ------------------------------------------------------------ frame

function Update()
    local cx, cy = read_cursor()
    read_desktop()
    t = t + frame
    if clicks.name and t - clicks.at >= clicks.wait then
        local name = clicks.name
        clicks.name = nil
        if name == 'pat' then Pat() else Eat() end
    end
    local dvx, dvy = window_push()
    local dragged = math.abs(window.vx) + math.abs(window.vy) > 30
    update_sleep(dragged)
    if sleep_state.asleep and t - sleep_state.start >= SLEEP.settle then
        -- Settled: the screen keeps the last frame, and the springs resume from it when she wakes.
        return 0
    end
    local gx, gy = gaze_target(cx, cy)
    local elapsed = action and (t - action_start) or 0
    if (action == 'eat' and elapsed >= ACTIONS.eat_seconds) or (action == 'pat' and elapsed >= ACTIONS.pat_seconds)
        or (action == 'write' and elapsed >= ACTIONS.write_seconds) then
        action = nil
        elapsed = 0
    end
    if not sleep_state.asleep then
        idle_events()
    end

    -- At the notebook: making a plan, a note, or resting while the ToDo window is shown.
    local at_desk = panel.visible and not sleep_state.asleep
    local pose = (writing or action == 'write' or at_desk) and 'Write' or 'Idle'
    local eyes, mouth, happy = nil, nil, false
    local lean_target = 1.0 * math.sin(TAU * t / 4.3) + 0.35 * math.sin(TAU * t / 1.9 + 1.3) + events.weight
    local tilt = events.tilt
    if pose == 'Write' then
        -- Resting at the notebook she now and then looks up (eyes on the cursor or the window, pen still).
        local looking_up = at_desk and not writing and action == nil and t < events.lookup_until
        local pen = looking_up and 0 or 1
        lean_target = 0.2 * math.sin(TAU * t / 4.3) + pen * 0.25 * math.sin(TAU * t * 1.6) + 0.5 * events.weight
        tilt = pen * 1.0 + 0.5 * events.tilt
        eyes = not looking_up and 'EyesDown' or nil
    end
    if action == 'eat' then
        pose = 'Eat'
        eyes = nil
        if elapsed >= 0.5 and elapsed < 0.9 then
            mouth = 'MouthOpen'
        elseif elapsed >= 0.9 and elapsed < 2.7 then
            eyes = 'EyesHappy'
            if math.floor((elapsed - 0.9) / 0.18) % 2 == 0 then
                mouth = 'MouthChew'
            end
        end
        tilt = ACTIONS.eat_tilt * smoothstep(0.6, 1.0, elapsed) * (1 - smoothstep(2.6, 3.0, elapsed))
            + 0.6 * math.sin(TAU * elapsed * 2.8) * smoothstep(0.9, 1.1, elapsed) * (1 - smoothstep(2.5, 2.7, elapsed))
    elseif action == 'pat' then
        happy = true
        tilt = ACTIONS.pat_tilt * math.sin(math.pi * math.min(1, elapsed / ACTIONS.pat_seconds))
    end

    local slept = sleep_state.asleep and (t - sleep_state.start) or 0
    if sleep_state.asleep then
        -- Dozing off: the torso settles upright, the head droops, the eyes close.
        lean_target = 0
        tilt = SLEEP.tilt * smoothstep(0, 2.5, slept)
        eyes = slept < 1.2 and 'EyesHalf' or 'EyesClosed'
        blink.next = t + 1
    end
    if t >= blink.next then
        blink.next = t + random_between(2.0, 6.0)
        if not happy and eyes ~= 'EyesHappy' then
            blink.start = t
        end
    end
    if blink.start then
        local b = t - blink.start
        if b < 0.05 or (b >= 0.13 and b < 0.18) then
            eyes = 'EyesHalf'
        elseif b < 0.13 then
            eyes = 'EyesClosed'
        else
            blink.start = nil
        end
    end
    if happy then
        eyes, mouth = 'FaceHappy', nil
    end

    -- A pat makes her rock from side to side, torso and head together, for a moment.
    local rocking = 0
    if action == 'pat' then
        rocking = smoothstep(0, 0.15, elapsed) * (1 - smoothstep(ACTIONS.pat_seconds - 0.3, ACTIONS.pat_seconds, elapsed))
        lean_target = lean_target + 3.0 * math.sin(TAU * 1.7 * elapsed) * (1 - elapsed / ACTIONS.pat_seconds)
    end

    -- Springs. The head follows the torso a little late; hair, ahoge, and fins are pushed opposite to the
    -- head's angular acceleration (inertia) and pulled toward a light, irregular wind.
    local head_v = head.v
    local head_sway = 1.6 * math.sin(TAU * t / 5.3) + 0.6 * math.sin(TAU * t / 2.2 + 1.0)
    if action == 'eat' or pose == 'Write' then
        head_sway = 0.4 * head_sway
    elseif sleep_state.asleep then
        head_sway = 0.2 * head_sway
    end
    -- Dragging: everything lags behind the window's acceleration, then swings back on its spring.
    lean.v = lean.v - DRAG.lean * dvx
    head.v = head.v - DRAG.head * dvx
    hair.v = hair.v - DRAG.hair * dvx
    ahoge.v = ahoge.v - DRAG.ahoge * dvx
    tail.v = tail.v + DRAG.tail * dvx
    fins.finl.v = fins.finl.v + DRAG.fin * dvy
    fins.finr.v = fins.finr.v + DRAG.fin * dvy
    -- Carried: the parts trail behind the window's velocity.
    local carry_x = math.max(-TRAIL.speed_max, math.min(TRAIL.speed_max, window.vx))
    local carry_y = math.max(-TRAIL.speed_max, math.min(TRAIL.speed_max, window.vy))
    lean_target = lean_target - TRAIL.lean * carry_x
    spring(lean, lean_target, action == 'pat' and 1.6 or 0.5, action == 'pat' and 0.35 or 0.8)
    local wind = sleep_state.asleep and 0.3 or 1
    spring(head, 1.2 * lean.x + head_sway + tilt - TRAIL.head * carry_x, 1.3, 0.55)
    local head_accel = head.v - head_v
    -- The back hair bends (hair.x is the sideways shift of the tips, in source pixels): it leans a little
    -- with the head, the tips trail the head's turns, and an irregular wind keeps it moving.
    hair.v = hair.v - 30 * head_accel
    spring(hair, 6 * head.x - TRAIL.hair * carry_x
        + wind * (16 * math.sin(TAU * t / 3.3 + 0.8) + 8 * math.sin(TAU * t / 1.37) + 4 * math.sin(TAU * t / 0.83)),
        0.7, 0.18)
    ahoge.v = ahoge.v - 3.0 * head_accel
    spring(ahoge, wind * (3 * math.sin(TAU * t / 2.1) + 1.5 * math.sin(TAU * t / 0.9 + 0.5)) - TRAIL.ahoge * carry_x,
        1.7, 0.12)
    spring(tail, wind * (6 * math.sin(TAU * t / 2.6) + 2 * math.sin(TAU * t / 1.15 + 2.0)) + TRAIL.tail * carry_x,
        0.9, 0.22)
    -- Fins: a little idle flutter, flapping while patted, drooping while eating; they lag the head's turns.
    for name, fin in pairs(fins) do
        local phase = name == 'finl' and 0 or 1.9
        local target = 1.5 * math.sin(TAU * t / 3.7 + phase) + TRAIL.fin * carry_y
        if sleep_state.asleep then
            target = 0.3 * target - 6 * smoothstep(0, 2.5, slept)
        elseif action == 'eat' then
            target = target - 8 * smoothstep(0.4, 0.9, elapsed) * (1 - smoothstep(2.7, 3.2, elapsed))
        elseif action == 'pat' then
            target = target + 9 * math.sin(TAU * 5 * elapsed) * (1 - elapsed / ACTIONS.pat_seconds)
        end
        fin.v = fin.v - FIN_UP[name] * 2.0 * head_accel
        spring(fin, target, 2.2, 0.18)
    end

    local moved = {}
    local hop = 0
    if happy and elapsed < 0.35 then
        hop = -round(3 * math.sin(math.pi * elapsed / 0.35))
        if elapsed < frame * 1.5 then
            ahoge.v = ahoge.v + 160
        end
    end
    set_offset('body', 0, hop, moved)
    -- While rocking, the head layers follow the neck (in whole pixels) as the torso turns around the chest;
    -- at other times the neck moves under half a skin pixel and the head is not shifted.
    local lr = math.rad(lean.x)
    local lc, ls = math.cos(lr), math.sin(lr)
    local hx = rocking * (rig.chest_x + lc * (rig.neck_x - rig.chest_x) - ls * (rig.neck_y - rig.chest_y) - rig.neck_x)
    local hy = rocking * (rig.chest_y + ls * (rig.neck_x - rig.chest_x) + lc * (rig.neck_y - rig.chest_y) - rig.neck_y)
    if rocking == 0 then
        hx, hy = 0, 0
    end
    hx, hy = round(hx), round(hy)
    set_offset('head', hx, hy + hop, moved)
    -- The ahoge's root rides on the head: rotate it around the neck by the head angle.
    local r = math.rad(head.x)
    local c, s = math.cos(r), math.sin(r)
    local ax = rig.neck_x + c * (rig.root_x - rig.neck_x) - s * (rig.root_y - rig.neck_y) - rig.root_x
    local ay = rig.neck_y + s * (rig.root_x - rig.neck_x) + c * (rig.root_y - rig.neck_y) - rig.root_y
    set_offset('ahoge', ax + hx, ay + hy + hop, moved)
    for name, root in pairs(rig.fin_roots) do
        local fx = rig.neck_x + c * (root[1] - rig.neck_x) - s * (root[2] - rig.neck_y) - root[1]
        local fy = rig.neck_y + s * (root[1] - rig.neck_x) + c * (root[2] - rig.neck_y) - root[2]
        set_offset(name, fx + hx, fy + hy + hop, moved)
    end
    local ease = 1 - math.exp(-GAZE.rate * frame)
    iris.x = iris.x + (gx - iris.x) * ease
    iris.y = iris.y + (gy - iris.y) * ease
    set_offset('iris', round(iris.x), round(iris.y), moved)

    -- The hair frames end at the largest prepared bend; a hard swing is eased into it instead of stopping there.
    local hair_limit = math.min(-angles.hair.first, angles.hair.first + angles.hair.step * (angles.hair.count - 1))
    local l = index(angles.lean, lean.x)
    local h = index(angles.head, head.x)
    local wanted = {
        ['Hair' .. index(angles.hair, hair_limit * math.tanh(hair.x / hair_limit))] = true,
        ['Tail' .. index(angles.tail, tail.x)] = true,
        ['Ahoge' .. index(angles.ahoge, head.x + ahoge.x)] = true,
        ['FinL' .. index(angles.fin, head.x + FIN_UP.finl * fins.finl.x)] = true,
        ['FinR' .. index(angles.fin, head.x + FIN_UP.finr * fins.finr.x)] = true,
        ['Lower' .. pose] = true,
        ['Upper' .. pose .. l] = true,
        ['Head' .. h] = true, ['Sclera' .. h] = true, ['IrisClip' .. h] = true, ['Iris' .. h] = true,
        ['Lashes' .. h] = true,
    }
    if eyes then
        wanted[eyes .. h] = true
    end
    if mouth then
        wanted[mouth .. h] = true
    end
    if pose ~= 'Idle' then
        wanted['Arms' .. pose .. l] = true
    end
    present(wanted, moved)
    return 0
end

-- ------------------------------------------------------------ actions (also called from ToDo\Main)

function Pat()
    if action ~= 'eat' then
        start_action('pat')
    end
end

function Eat()
    if action ~= 'eat' then
        start_action('eat')
    end
end

function StartWriting()
    writing = true
end

function StopWriting()
    writing = false
end

local function click(name)
    if t >= clicks.ignored_until then
        clicks.name, clicks.at = name, t
    end
end

function ClickHead()
    click('pat')
end

function ClickBody()
    click('eat')
end

-- Opens the chat bubble (ToDo\Companion) beside her, on the left when there is room, or closes it.
local COMPANION_W = 300    -- BubbleW in Companion.ini
function ToggleCompanion()
    -- Read the bubble's window now, not as of the measure's last scheduled update.
    SKIN:Bang('!UpdateMeasure', 'MeasureCompanionWindow')
    local bubble = desktop_state('MeasureCompanionWindow')
    if bubble and bubble.visible then
        SKIN:Bang('!DeactivateConfig', 'ToDo\\Companion')
        return
    end
    local x = window.origin_x - COMPANION_W - 4
    if x < variable('WORKAREAX') then
        x = window.origin_x + variable('MascotWindowW') + 4
    end
    local y = math.max(variable('WORKAREAY'), window.origin_y + 8)
    SKIN:Bang('!ActivateConfig', 'ToDo\\Companion', 'Companion.ini')
    SKIN:Bang('!Move', x, y, 'ToDo\\Companion')
end

-- A double-click on the tail or a fin flicks it twice but toggles the bubble once.
local function toggle_bubble_by_click()
    if t - clicks.bubble_at >= clicks.wait then
        clicks.bubble_at = t
        ToggleCompanion()
    end
end

function ClickTail()
    tail.v = tail.v + (math.random() < 0.5 and -1 or 1) * 140
    toggle_bubble_by_click()
end

function ClickFin(side)
    fins[side].v = fins[side].v + 220
    toggle_bubble_by_click()
end

function ToggleTodo()
    clicks.name = nil
    clicks.ignored_until = t + clicks.wait
    todo_shown = not todo_shown
    SKIN:Bang(todo_shown and '!Show' or '!Hide', 'ToDo\\Main')
end
