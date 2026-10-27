-- The mascot's speech bubble (Companion.ini): today's short conversation from todo.exe companion.
-- Conversations are never written here; every change is a todo.exe command. The mascot opens the skin
-- beside herself (ToggleCompanion()) and smiles (Pat()) when an answer arrives.
-- Input: todo.exe prompt --dock-below, a window right under the bubble that follows it. It opens whenever
-- the bubble is idle; Esc closes it and the header's "ask" opens it again.

local protocol   -- todo_protocol.lua, loaded in Initialize()
local SCROLL_STEP = 40
local SHOWN_TURNS = 8      -- todo.exe sends the latest 8 turns

local turns      -- { role, content, tools = { name, ... } }
local pending_tools
local busy
local prompting
local scroll     -- pixels the conversation is scrolled up from its latest line
local status

function Initialize()
    protocol = dofile(SKIN:GetVariable('@') .. 'Scripts\\todo_protocol.lua')
    turns = {}
    pending_tools = {}
    busy = false
    prompting = false
    scroll = 0
    status = ''
end

-- ------------------------------------------------------------ protocol

-- todo.exe output for this view: line breaks kept, TABs as four spaces.
local function parse_records(output)
    return protocol.parse_records(output, { newline = '\n', tab = '    ' })
end

local function num(name)
    return tonumber(SKIN:GetVariable(name)) or 0
end

-- ------------------------------------------------------------ layout

local function conversation_text()
    local parts = {}
    for _, turn in ipairs(turns) do
        if turn.role == 'user' then
            parts[#parts + 1] = '> ' .. (turn.content:gsub('\n', ' '))
        else
            for _, name in ipairs(turn.tools) do
                parts[#parts + 1] = '    looked up: ' .. name
            end
            -- The bubble shows plain text; leftover Markdown emphasis and code marks are dropped.
            parts[#parts + 1] = (turn.content:gsub('%*%*', ''):gsub('`', ''))
        end
    end
    if #parts == 0 then
        return 'Hi! Ask me anything, or ask me to look something up.'
    end
    return table.concat(parts, '\n')
end

local function layout()
    local pad_x, pad_y = num('PadX'), num('PadY')
    local width = num('BubbleW')
    local inner = width - 2 * pad_x
    local y = pad_y + num('HeaderH')

    SKIN:Bang('!SetOption', 'Conversation', 'Text', conversation_text())
    SKIN:Bang('!UpdateMeter', 'Conversation')
    local text_h = SKIN:GetMeter('Conversation'):GetH()
    local view_h = math.min(text_h, num('MaxTextH'))
    local max_scroll = text_h - view_h
    scroll = math.max(0, math.min(scroll, max_scroll))
    -- Bottom-aligned: the latest line is visible unless the user scrolled up.
    SKIN:Bang('!SetOption', 'Conversation', 'Y', view_h - text_h + scroll)
    SKIN:Bang('!SetOption', 'Viewport', 'Y', y)
    SKIN:Bang('!SetOption', 'Viewport', 'Shape',
        string.format('Rectangle 0,0,%d,%d | Fill Color 255,255,255,255 | StrokeWidth 0', inner, view_h))
    -- Scrollbar thumb at the right edge when older lines are hidden; scroll = 0 is the bottom.
    if max_scroll > 0 then
        local thumb_h = math.max(16, view_h * view_h / text_h)
        local thumb_y = y + (view_h - thumb_h) * (1 - scroll / max_scroll)
        SKIN:Bang('!SetOption', 'ScrollThumb', 'Shape', string.format(
            'Rectangle %d,%d,3,%d,1.5 | Fill Color %s | StrokeWidth 0', width - pad_x + 4, thumb_y, thumb_h,
            SKIN:GetVariable('ColorDim')))
        SKIN:Bang('!ShowMeter', 'ScrollThumb')
    else
        SKIN:Bang('!HideMeter', 'ScrollThumb')
    end
    y = y + view_h + 6

    local line = busy and 'Thinking...' or status
    SKIN:Bang('!SetOption', 'Status', 'Text', line)
    SKIN:Bang('!SetOption', 'Status', 'Y', y)
    if line ~= '' then
        SKIN:Bang('!ShowMeter', 'Status')
        SKIN:Bang('!UpdateMeter', 'Status')
        y = y + SKIN:GetMeter('Status'):GetH() + 4
    else
        SKIN:Bang('!HideMeter', 'Status')
    end

    SKIN:Bang(prompting and '!HideMeter' or '!ShowMeter', 'ButtonAsk')
    y = y + pad_y - 4

    SKIN:Bang('!SetOption', 'Bubble', 'Shape', string.format(
        'Rectangle 0.5,0.5,%d,%d,%s | Fill Color %s | StrokeWidth 1 | Stroke Color %s',
        width - 1, y - 1, SKIN:GetVariable('Radius'), SKIN:GetVariable('ColorBg'), SKIN:GetVariable('ColorStroke')))
    SKIN:Bang('!UpdateMeter', '*')
    SKIN:Bang('!Redraw')
end

function Update()
    return #turns
end

-- ------------------------------------------------------------ actions

function Start()
    layout()
    busy = true
    protocol.run('MeasureCompanionRun', { 'companion', 'show' })
end

function Scroll(direction)
    scroll = scroll - direction * SCROLL_STEP
    layout()
end

function NewChat()
    if busy then
        return
    end
    busy = true
    status = ''
    layout()
    protocol.run('MeasureCompanionRun', { 'companion', 'new' })
end

-- Opens the input under the bubble (also the header's "ask").
function OpenInput()
    if prompting then
        return
    end
    prompting = true
    layout()
    protocol.run('MeasureCompanionPrompt', protocol.prompt_colors({
        'prompt', 'Ask her  (Enter: send, Esc: close)',
        '--anchor', SKIN:GetVariable('CURRENTPATH') .. SKIN:GetVariable('CURRENTFILE'),
        '--offset-x', '0', '--offset-y', '0', '--dock-below',
    }))
end

function OnPromptFinished()
    prompting = false
    local reopen = false
    for _, record in ipairs(parse_records(SKIN:GetMeasure('MeasureCompanionPrompt'):GetStringValue())) do
        if record[1] == 'INPUT' then
            if (record[2] or ''):match('%S') and not busy then
                Send(record[2])
            else
                reopen = true  -- Enter on an empty line keeps the input open
            end
        elseif record[1] == 'ERR' then
            status = 'Error: ' .. (record[2] or '')
        end
    end
    layout()
    if reopen then
        OpenInput()
    end
end

-- Also callable as !CommandMeasure MeasureCompanion "Send('...')" "ToDo\Companion"; the input uses it.
function Send(text)
    if busy or not text:match('%S') then
        return
    end
    turns[#turns + 1] = { role = 'user', content = text, tools = {} }
    busy = true
    status = ''
    scroll = 0
    layout()
    protocol.run('MeasureCompanionRun', { 'companion', 'send', text })
end

function OnRunFinished()
    busy = false
    local received = {}
    local tools = {}
    local answered = false
    local error_text = nil
    for _, record in ipairs(parse_records(SKIN:GetMeasure('MeasureCompanionRun'):GetStringValue())) do
        if record[1] == 'TOOL' then
            tools[#tools + 1] = record[2]
        elseif record[1] == 'TURN' then
            received[#received + 1] = { role = record[2], content = record[3] or '', tools = record[2] == 'assistant' and tools or {} }
            if record[2] == 'assistant' then
                tools = {}
            end
        elseif record[1] == 'OK' and record[2] and record[2]:match('^sent ') then
            answered = true
        elseif record[1] == 'ERR' then
            error_text = record[2] or ''
        end
    end
    if error_text then
        status = 'Error: ' .. error_text
    else
        turns = received
        status = ''
    end
    while #turns > SHOWN_TURNS do
        table.remove(turns, 1)
    end
    scroll = 0
    layout()
    -- Ready for the next message.
    OpenInput()
    if answered then
        SKIN:Bang('!CommandMeasure', 'MeasureMascot', 'Pat()', 'ToDo\\Mascot')
    end
end
