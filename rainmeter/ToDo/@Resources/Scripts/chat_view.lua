-- The ask bar, the conversation, the conversation list, the message line, and the background of Main.ini.
-- task_view.lua or plan_view.lua calls SetTop(y) with the bottom of the area above.
-- Conversations are never written here; every change is a todo.exe command.

local protocol   -- todo_protocol.lua, loaded in Initialize()
local SCROLL_STEP = 48

local list_rows
local ui_state_file
local max_chat_height
local top
local current_chat
local title
local session_skill
local skills
local skill
local with_tasks
local chat_open
local list_open
local chats
local list_page
local turns          -- { role, content, tools = { { name, summary, status } } }
local scroll
local scroll_pending
local viewport_height
local busy
local prompting
local message
local usage_text

function Initialize()
    protocol = dofile(SKIN:GetVariable('@') .. 'Scripts\\todo_protocol.lua')
    list_rows = SELF:GetNumberOption('ListRows', 8)
    ui_state_file = SELF:GetOption('UiStateFile')
    max_chat_height = SELF:GetNumberOption('MaxChatHeight', 240)
    top = 0
    current_chat = SKIN:GetVariable('CurrentChat', '')
    title = ''
    session_skill = ''
    skills = {}
    skill = SKIN:GetVariable('Skill', '')
    with_tasks = SKIN:GetVariable('WithTasks', '0') == '1'
    chat_open = SKIN:GetVariable('ChatOpen', '1') == '1'
    list_open = false
    chats = {}
    list_page = 1
    turns = {}
    scroll = 0
    scroll_pending = false
    viewport_height = 0
    busy = false
    prompting = false
    message = ''
    usage_text = ''
end

function Update()
    return #turns
end

-- ------------------------------------------------------------ protocol

-- todo.exe output for this view: line breaks kept, TABs as four spaces.
local function parse_records(output)
    return protocol.parse_records(output, { newline = '\n', tab = '    ' })
end

local function persist(key, value)
    SKIN:Bang('!SetVariable', key, value)
    SKIN:Bang('!WriteKeyValue', 'Variables', key, value, ui_state_file)
end

local function num(name)
    return tonumber(SKIN:GetVariable(name)) or 0
end

local function color(name)
    return SKIN:GetVariable(name)
end

local function anchor()
    return SKIN:GetVariable('CURRENTPATH') .. SKIN:GetVariable('CURRENTFILE')
end

-- ------------------------------------------------------------ rendering

local function conversation_text()
    local parts = {}
    for _, turn in ipairs(turns) do
        if turn.role == 'user' then
            parts[#parts + 1] = (#parts > 0 and '\n' or '') .. 'You  ' .. turn.content
        else
            local lines = {}
            for _, tool in ipairs(turn.tools) do
                lines[#lines + 1] = '    ' .. tool.status .. '  ' .. tool.name .. '  ' .. tool.summary
            end
            lines[#lines + 1] = 'AI  ' .. turn.content
            parts[#parts + 1] = table.concat(lines, '\n')
        end
    end
    return table.concat(parts, '\n')
end

-- Height of the conversation text up to (not including) turn `last_user`.
local function earlier_height(last_user)
    if last_user <= 1 then
        return 0
    end
    SKIN:Bang('!ShowMeter', 'ChatText')
    local saved = turns
    local earlier = {}
    for i = 1, last_user - 1 do
        earlier[i] = saved[i]
    end
    turns = earlier
    SKIN:Bang('!SetOption', 'ChatText', 'Text', conversation_text() .. '\n')
    turns = saved
    SKIN:Bang('!UpdateMeter', 'ChatText')
    return SKIN:GetMeter('ChatText'):GetH()
end

local function apply_scroll(text_height)
    scroll = math.max(0, math.min(scroll, math.max(0, text_height - viewport_height)))
    SKIN:Bang('!SetOption', 'ChatText', 'Y', -scroll)
    SKIN:Bang('!UpdateMeter', 'ChatText')
end

local function skill_label()
    local name = current_chat ~= '' and session_skill or skill
    return name ~= '' and name or 'no skill'
end

local function layout_chips(cy)
    local right = num('SkinWidth') - num('PadX') - 28
    SKIN:Bang('!SetOption', 'ButtonChats', 'Y', cy - 8.5)
    SKIN:Bang('!SetOption', 'ButtonTasks', 'X', right)
    SKIN:Bang('!SetOption', 'ButtonTasks', 'Y', cy)
    SKIN:Bang('!SetOption', 'ButtonTasks', 'FontColor', color(with_tasks and 'ColorAccent' or 'ColorDim'))
    SKIN:Bang('!SetOption', 'ButtonTasks', 'SolidColor', color(with_tasks and 'ColorSegOn' or 'ColorPanel'))
    SKIN:Bang('!SetOption', 'ButtonTasks', 'ToolTipText',
        with_tasks and 'Task lists are included. Click: leave them out' or 'Click: include the task lists')
    SKIN:Bang('!UpdateMeter', 'ButtonTasks')
    SKIN:Bang('!SetOption', 'ButtonSkill', 'X', right - SKIN:GetMeter('ButtonTasks'):GetW() - 5)
    SKIN:Bang('!SetOption', 'ButtonSkill', 'Y', cy)
    SKIN:Bang('!SetOption', 'ButtonSkill', 'Text', skill_label())
    SKIN:Bang('!SetOption', 'ButtonSkill', 'FontColor', color(skill_label() ~= 'no skill' and 'ColorText' or 'ColorDim'))
end

local function show_at(meter, y)
    SKIN:Bang('!SetOption', meter, 'Y', y)
    SKIN:Bang('!ShowMeter', meter)
end

-- Scrolls so that the latest question is at the top of the viewport. Measuring needs ChatText shown,
-- so layout() does it while the conversation is visible.
local function scroll_to_latest()
    local last_user = #turns
    while last_user > 1 and turns[last_user].role ~= 'user' do
        last_user = last_user - 1
    end
    scroll = earlier_height(last_user)
end

local function layout()
    local y = top
    show_at('AskLine', y)
    y = y + 8
    local cy = y + 10
    show_at('AskPrompt', cy)
    show_at('AskPlaceholder', cy)
    local placeholder = busy and 'Thinking...' or (current_chat == '' and 'Ask...' or 'Follow up...')
    SKIN:Bang('!SetOption', 'AskPlaceholder', 'Text', placeholder)
    layout_chips(cy)
    y = y + 22

    SKIN:Bang('!HideMeterGroup', 'ChatList')
    for _, name in ipairs({ 'ChatViewport', 'ChatText', 'ChatFoot', 'ButtonNewChat', 'ButtonFold' }) do
        SKIN:Bang('!HideMeter', name)
    end

    if list_open then
        local row_h = num('RowH')
        y = y + 4
        show_at('ListNew', y + row_h / 2)
        y = y + row_h
        local pages = math.max(1, math.ceil(#chats / list_rows))
        list_page = math.max(1, math.min(list_page, pages))
        for i = 1, list_rows do
            local chat = chats[(list_page - 1) * list_rows + i]
            if chat then
                local marker = chat.id == current_chat and '>' or ' '
                local skill_note = chat.skill ~= '' and ('  [' .. chat.skill .. ']') or ''
                SKIN:Bang('!SetOption', 'ListRow' .. i, 'Text',
                    marker .. (chat.created:sub(6, 16):gsub('T', ' ')) .. '  ' .. chat.title .. '  (' .. chat.turns .. ')' .. skill_note)
                show_at('ListRow' .. i, y + row_h / 2)
                y = y + row_h
            end
        end
        if pages > 1 then
            for _, name in ipairs({ 'ListPrev', 'ListPage', 'ListNext' }) do
                show_at(name, y + 9)
            end
            SKIN:Bang('!SetOption', 'ListPage', 'Text', list_page .. '/' .. pages)
            y = y + 18
        end
    elseif #turns > 0 then
        if chat_open then
            -- A hidden meter reports no height, so show it before measuring.
            SKIN:Bang('!ShowMeter', 'ChatViewport')
            SKIN:Bang('!ShowMeter', 'ChatText')
            if scroll_pending then
                scroll_to_latest()
                scroll_pending = false
            end
            SKIN:Bang('!SetOption', 'ChatText', 'Text', conversation_text())
            SKIN:Bang('!UpdateMeter', 'ChatText')
            local text_height = SKIN:GetMeter('ChatText'):GetH()
            viewport_height = math.min(text_height, max_chat_height)
            SKIN:Bang('!SetOption', 'ChatViewport', 'Shape',
                'Rectangle 0,0,' .. (num('SkinWidth') - 2 * num('PadX')) .. ',' .. viewport_height ..
                ' | Fill Color 255,255,255,255 | StrokeWidth 0')
            show_at('ChatViewport', y + 4)
            SKIN:Bang('!ShowMeter', 'ChatText')
            apply_scroll(text_height)
            y = y + 4 + viewport_height + 4
        end
        local foot = usage_text ~= '' and usage_text or (title .. '  (' .. #turns .. ' turns)')
        if not chat_open then
            foot = title .. '  (' .. #turns .. ' turns, folded)'
        end
        SKIN:Bang('!SetOption', 'ChatFoot', 'Text', foot)
        SKIN:Bang('!SetOption', 'ButtonFold', 'Text', chat_open and '^' or 'v')
        show_at('ChatFoot', y + 9)
        show_at('ButtonNewChat', y + 9)
        show_at('ButtonFold', y + 9)
        y = y + 18
    end

    if message ~= '' then
        SKIN:Bang('!SetOption', 'MeterMessage', 'Text', message)
        show_at('MeterMessage', y + 2)
        SKIN:Bang('!UpdateMeter', 'MeterMessage')
        y = y + 2 + SKIN:GetMeter('MeterMessage'):GetH()
    else
        SKIN:Bang('!HideMeter', 'MeterMessage')
    end
    y = y + num('PadY') - 2

    SKIN:Bang('!SetOption', 'MeterBackground', 'Shape',
        string.format('Rectangle 0.5,0.5,%d,%d,%s | Fill Color %s | Stroke Color %s | StrokeWidth 1',
            num('SkinWidth') - 1, math.floor(y), SKIN:GetVariable('Radius'), color('ColorBg'), color('ColorStroke')))
    SKIN:Bang('!UpdateMeter', '*')
    SKIN:Bang('!Redraw')
end

-- ------------------------------------------------------------ entry points

function Start()
    protocol.run('MeasureSkillList', { 'skill', 'list' })
    if current_chat ~= '' then
        busy = true
        protocol.run('MeasureChatRun', { 'chat', 'show', current_chat })
    end
end

function SetTop(y)
    top = y
    layout()
end

function ShowMessage(text)
    message = text or ''
    layout()
end

function OnSkillsFinished()
    skills = {}
    for _, record in ipairs(parse_records(SKIN:GetMeasure('MeasureSkillList'):GetStringValue())) do
        if record[1] == 'SKILL' then
            skills[#skills + 1] = record[2]
        elseif record[1] == 'ERR' then
            message = 'Error: ' .. (record[2] or '')
        end
    end
    layout()
end

function CycleSkill()
    if busy then
        return
    end
    local options = { '' }
    for _, name in ipairs(skills) do
        options[#options + 1] = name
    end
    local from = current_chat ~= '' and session_skill or skill
    local index = 1
    for i, name in ipairs(options) do
        if name == from then
            index = i
        end
    end
    skill = options[index % #options + 1]
    persist('Skill', skill)
    if current_chat ~= '' then
        current_chat = ''
        turns = {}
        title = ''
        usage_text = ''
        persist('CurrentChat', '')
        message = 'New conversation with ' .. (skill ~= '' and ('skill ' .. skill) or 'no skill')
    end
    layout()
end

function ToggleTasks()
    with_tasks = not with_tasks
    persist('WithTasks', with_tasks and '1' or '0')
    layout()
end

function ToggleFold()
    chat_open = not chat_open
    persist('ChatOpen', chat_open and '1' or '0')
    layout()
end

function Scroll(direction)
    scroll = scroll + direction * SCROLL_STEP
    apply_scroll(SKIN:GetMeter('ChatText'):GetH())
    SKIN:Bang('!Redraw')
end

function NewChat()
    if busy then
        return
    end
    current_chat = ''
    title = ''
    session_skill = ''
    turns = {}
    usage_text = ''
    list_open = false
    persist('CurrentChat', '')
    message = ''
    layout()
end

-- ------------------------------------------------------------ asking

function OnAskClick()
    if busy or prompting then
        return
    end
    prompting = true
    local caption = current_chat == '' and ('New question, ' .. skill_label()) or 'Follow-up'
    local default_y = top + 8
    protocol.run('MeasureChatPrompt', protocol.prompt_colors({
        'prompt', caption .. '  (Enter: send, Esc: cancel)',
        '--anchor', anchor(),
        '--offset-x', SKIN:GetVariable('ChatPromptX', '0'),
        '--offset-y', SKIN:GetVariable('ChatPromptY', tostring(default_y)),
    }))
end

function OnPromptFinished()
    prompting = false
    for _, record in ipairs(parse_records(SKIN:GetMeasure('MeasureChatPrompt'):GetStringValue())) do
        if record[1] == 'INPUT' then
            Send(record[2])
        elseif record[1] == 'POSITION' then
            persist('ChatPromptX', record[2])
            persist('ChatPromptY', record[3])
        elseif record[1] == 'ERR' then
            ShowMessage('Error: ' .. (record[2] or ''))
        end
    end
end

function Send(text)
    if busy or text == nil or text:match('^%s*$') then
        return
    end
    busy = true
    list_open = false
    chat_open = true
    persist('ChatOpen', '1')
    turns[#turns + 1] = { role = 'user', content = text, tools = {} }
    turns[#turns + 1] = { role = 'assistant', content = '...', tools = {} }
    usage_text = ''
    message = 'Thinking...'
    local args
    if current_chat == '' then
        args = { 'ask' }
        if skill ~= '' then
            args[#args + 1] = '--skill'
            args[#args + 1] = skill
        end
    else
        args = { 'chat', 'send', current_chat }
    end
    if with_tasks then
        args[#args + 1] = '--with-tasks'
    end
    -- The write confirmation window is placed over this skin.
    args[#args + 1] = '--anchor'
    args[#args + 1] = anchor()
    args[#args + 1] = text
    scroll_pending = true
    layout()
    protocol.run('MeasureChatRun', args)
end

function OnChatFinished()
    busy = false
    local records = parse_records(SKIN:GetMeasure('MeasureChatRun'):GetStringValue())
    local loaded = {}
    local pending_tools = {}
    local error_text = nil
    local usage = ''
    local loaded_id, loaded_title, loaded_skill = nil, '', ''
    for _, record in ipairs(records) do
        local kind = record[1]
        if kind == 'CHAT' then
            loaded_id, loaded_title, loaded_skill = record[2], record[3] or '', record[6] or ''
        elseif kind == 'TOOL' then
            pending_tools[#pending_tools + 1] = { name = record[2] or '', summary = record[3] or '', status = record[4] or '' }
        elseif kind == 'TURN' then
            loaded[#loaded + 1] = { role = record[2], content = record[3] or '', tools = pending_tools }
            pending_tools = {}
        elseif kind == 'USAGE' then
            local tokens_in = tonumber(record[2]) or 0
            local tools = loaded[#loaded] and #loaded[#loaded].tools or 0
            usage = string.format('%d tools | %.1fk / %s tokens | %.1fs', tools, tokens_in / 1000, record[3] or '?',
                (tonumber(record[4]) or 0) / 1000)
        elseif kind == 'ERR' then
            error_text = 'Error: ' .. (record[2] or '')
        end
    end
    if error_text then
        -- Drop the optimistic question and placeholder answer of a failed send.
        if turns[#turns] and turns[#turns].content == '...' then
            turns[#turns] = nil
            turns[#turns] = nil
        end
        -- A conversation that cannot be loaded is forgotten.
        if current_chat ~= '' and #turns == 0 then
            persist('CurrentChat', '')
            current_chat = ''
        end
        message = error_text
        layout()
        return
    end
    if loaded_id then
        current_chat, title, session_skill = loaded_id, loaded_title, loaded_skill
        persist('CurrentChat', current_chat)
    end
    turns = loaded
    usage_text = usage
    message = ''
    scroll_pending = true
    layout()
    -- An answer is a meal for the mascot (ToDo\Mascot).
    if usage ~= '' then
        SKIN:Bang('!CommandMeasure', 'MeasureMascot', 'Eat()', 'ToDo\\Mascot')
    end
    -- Tools may have changed tasks or stored a new plan.
    local latest = turns[#turns]
    if usage ~= '' and latest and #latest.tools > 0 then
        local proposed = nil
        for _, tool in ipairs(latest.tools) do
            if tool.name == 'propose_plan' and tool.status == 'ok' then
                proposed = tool.summary:match('^proposed %d+%-W') and 'week' or 'day'
            end
        end
        if proposed then
            SKIN:Bang('!CommandMeasure', 'MeasurePlan', "Focus('" .. proposed .. "')")
            SKIN:Bang('!CommandMeasure', 'MeasureTasks', "SetView('plan')")
        else
            SKIN:Bang('!CommandMeasure', 'MeasureTasks', 'Tick()')
        end
    end
end

-- ------------------------------------------------------------ conversation list

function ToggleList()
    list_open = not list_open
    if list_open then
        protocol.run('MeasureChatList', { 'chat', 'list' })
    end
    layout()
end

function OnListFinished()
    chats = {}
    for _, record in ipairs(parse_records(SKIN:GetMeasure('MeasureChatList'):GetStringValue())) do
        if record[1] == 'CHAT' then
            chats[#chats + 1] = { id = record[2], title = record[3], created = record[4], turns = record[5],
                skill = record[6] or '' }
        elseif record[1] == 'ERR' then
            message = 'Error: ' .. (record[2] or '')
        end
    end
    -- Newest first.
    table.sort(chats, function(a, b) return a.created > b.created end)
    layout()
end

function ChangeListPage(delta)
    list_page = list_page + delta
    layout()
end

function SelectRow(row)
    local chat = chats[(list_page - 1) * list_rows + row]
    if not chat or busy then
        return
    end
    current_chat = chat.id
    persist('CurrentChat', current_chat)
    list_open = false
    usage_text = ''
    busy = true
    layout()
    protocol.run('MeasureChatRun', { 'chat', 'show', current_chat })
end
