-- Header, the Plan | Tasks tabs, and the Tasks tab (day / week / long lists) of Main.ini.
-- plan_view.lua draws the Plan tab below the header; chat_view.lua draws the ask bar, the conversation,
-- and the background below whichever is shown.
-- Content files are never written here; every change is a todo.exe command.

local protocol   -- todo_protocol.lua, loaded in Initialize()
local NEXT_STATUS = { todo = 'doing', doing = 'done', done = 'todo', blocked = 'todo', dropped = 'todo' }
local ALT_STATUS = { blocked = 'dropped', dropped = 'todo' }
local NEXT_PLACE = { any = 'lab', lab = 'class', class = 'remote', remote = 'any' }
local PLACE_LETTER = { lab = 'L', class = 'C', remote = 'R' }
local PLACE_COLOR = { lab = 'ColorLab', class = 'ColorClass', remote = 'ColorRemote' }
local VIEW_NAME = { day = 'Today', week = 'Week', long = 'Long-term' }
-- Segment meter, left offset inside the segmented control, and width.
local TABS = { plan = { 'SegPlan', 2, 44 }, tasks = { 'SegTasks', 48, 46 } }
local SCOPE_CHIP = { day = 'TaskScopeDay', week = 'TaskScopeWeek', long = 'TaskScopeLong' }
local TOOLBAR_H = 24
local NEXT_STYLE = { a = 'b', b = 'c', c = 'a' }
local WEEKDAY = { 'Sun', 'Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat' }
local EMPTY_SHAPE = 'Rectangle 0,0,0,0 | StrokeWidth 0 | Fill Color 0,0,0,0'
local MAX_BARS = 24

local row_count
local ui_state_file
local view   -- plan | tasks
local scope  -- day | week | long, the list shown on the Tasks tab
local page
local tasks
local message
local input_mode
local input_task_id
local today

function Initialize()
    protocol = dofile(SKIN:GetVariable('@') .. 'Scripts\\todo_protocol.lua')
    row_count = SELF:GetNumberOption('RowCount', 10)
    ui_state_file = SELF:GetOption('UiStateFile')
    view = SKIN:GetVariable('View', 'plan')
    scope = SKIN:GetVariable('TaskScope', 'day')
    -- Earlier versions stored the list name itself in View.
    if SCOPE_CHIP[view] then
        scope = view
        view = 'tasks'
    end
    if not TABS[view] then
        view = 'plan'
    end
    if not SCOPE_CHIP[scope] then
        scope = 'day'
    end
    page = 1
    tasks = {}
    message = ''
    today = os.date('%Y-%m-%d')
end

function Update()
    return #tasks
end

-- ------------------------------------------------------------ protocol

-- todo.exe output for this view: task rows are single lines.
local function parse_records(output)
    return protocol.parse_records(output, { newline = ' ', tab = ' ' })
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

-- chat_view.lua owns the message line.
local function notify(text)
    local cleaned = (text or ''):gsub('%]%]', '] ]'):gsub('[\r\n]', ' ')
    SKIN:Bang('!CommandMeasure', 'MeasureChat', 'ShowMessage([[' .. cleaned .. ']])')
end

local function summarize(records)
    for _, record in ipairs(records) do
        if record[1] == 'ERR' then
            return 'Error: ' .. (record[2] or '')
        end
    end
    for _, record in ipairs(records) do
        if record[1] == 'ROLLOVER' and record[6] == 'yes' and (record[4] ~= '0' or record[5] ~= '0') then
            return 'Rolled ' .. record[4] .. ' day / ' .. record[5] .. ' week tasks from ' .. record[2]
        end
    end
    return nil
end

-- ------------------------------------------------------------ geometry

-- Top of the area below the header and the progress bars; plan_view.lua uses the same formula.
local function content_top()
    return num('PadY') + num('HeaderH') + 4 + num('BarH') + 8
end

local function iso_week(t)
    local function weeks_in(year)
        local function p(y)
            return (y + math.floor(y / 4) - math.floor(y / 100) + math.floor(y / 400)) % 7
        end
        return (p(year) == 4 or p(year - 1) == 3) and 53 or 52
    end
    local weekday = (t.wday + 5) % 7 + 1 -- Monday = 1
    local week = math.floor((t.yday - weekday + 10) / 7)
    if week < 1 then
        return weeks_in(t.year - 1)
    elseif week > weeks_in(t.year) then
        return 1
    end
    return week
end

-- ------------------------------------------------------------ rendering

local function render_header()
    local t = os.date('*t')
    today = os.date('%Y-%m-%d')
    SKIN:Bang('!SetOption', 'MeterDate', 'Text', WEEKDAY[t.wday] .. ' ' .. os.date('%m-%d'))
    local tab = TABS[view]
    SKIN:Bang('!SetOption', 'SegSelected', 'X', '(#SegX#+' .. tab[2] .. ')')
    SKIN:Bang('!SetOption', 'SegSelected', 'Shape',
        'Rectangle 0,0,' .. tab[3] .. ',16,4 | Fill Color ' .. color('ColorSegOn') .. ' | StrokeWidth 0')
    for name, other in pairs(TABS) do
        SKIN:Bang('!SetOption', other[1], 'FontColor', color(name == view and 'ColorAccent' or 'ColorDim'))
    end
    SKIN:Bang('!UpdateMeter', 'MeterDate')
    SKIN:Bang('!UpdateMeter', 'SegSelected')
end

local function counted(task)
    return task.status ~= 'dropped' and task.status ~= 'rolled'
end

local function render_sub()
    local done, total = 0, 0
    for _, task in ipairs(tasks) do
        if counted(task) then
            total = total + 1
            if task.status == 'done' then
                done = done + 1
            end
        end
    end
    SKIN:Bang('!SetOption', 'MeterSub', 'Text',
        string.format('W%02d  %s %d/%d', iso_week(os.date('*t')), scope, done, total))
end

local function render_bars()
    local shown = {}
    for _, task in ipairs(tasks) do
        if counted(task) and #shown < MAX_BARS then
            shown[#shown + 1] = task
        end
    end
    local width = num('SkinWidth') - 2 * num('PadX')
    local gap = num('BarGap')
    local height = num('BarH')
    local segment = #shown > 0 and (width - gap * (#shown - 1)) / #shown or 0
    for i = 1, MAX_BARS do
        local option = i == 1 and 'Shape' or ('Shape' .. i)
        local task = shown[i]
        local value = EMPTY_SHAPE
        if task then
            local fill = task.status == 'done' and 'ColorDone' or (task.status == 'doing' and 'ColorAccent' or 'ColorLine')
            value = string.format('Rectangle %.1f,0,%.1f,%d,%.1f | Fill Color %s | StrokeWidth 0',
                (i - 1) * (segment + gap), segment, height, height / 2, color(fill))
        end
        SKIN:Bang('!SetOption', 'TaskBars', option, value)
    end
    SKIN:Bang('!UpdateMeter', 'TaskBars')
end

-- Shape, Shape2 (status mark), Shape3 (half fill or slash), and the hit area.
local function status_shapes(status, height)
    local cx, cy, r = 8, height / 2, 4.5
    local hit = 'Rectangle 0,0,18,' .. height .. ' | StrokeWidth 0 | Fill Color 0,0,0,1'
    local function ring(name)
        return string.format('Ellipse %g,%g,%g | StrokeWidth 1.3 | Stroke Color %s | Fill Color 0,0,0,0', cx, cy, r, color(name))
    end
    if status == 'done' then
        return hit, string.format('Ellipse %g,%g,3.5 | StrokeWidth 0 | Fill Color %s', cx, cy, color('ColorDone')), EMPTY_SHAPE
    elseif status == 'doing' then
        return hit, ring('ColorAccent'), 'Path HalfDisc | StrokeWidth 0 | Fill Color ' .. color('ColorAccent')
    elseif status == 'blocked' or status == 'dropped' then
        local name = status == 'blocked' and 'ColorBlocked' or 'ColorDim'
        return hit, ring(name), string.format('Line %g,%g,%g,%g | StrokeWidth 1.3 | Stroke Color %s',
            cx - 3.2, cy + 3.2, cx + 3.2, cy - 3.2, color(name))
    end
    return hit, ring('ColorDim'), EMPTY_SHAPE
end

local function place_shape(place, height)
    local cy = height / 2
    if PLACE_COLOR[place] then
        return string.format('Ellipse 8,%g,8 | StrokeWidth 0 | Fill Color %s', cy, color(PLACE_COLOR[place]))
    end
    return string.format('Ellipse 8,%g,3 | StrokeWidth 1 | Stroke Color %s | Fill Color 0,0,0,0', cy, color('ColorLine'))
end

local function format_minutes(minutes)
    minutes = tonumber(minutes)
    if not minutes then
        return ''
    elseif minutes < 60 then
        return minutes .. 'm'
    elseif minutes % 60 == 0 then
        return (minutes / 60) .. 'h'
    end
    return math.floor(minutes / 60) .. 'h' .. (minutes % 60) .. 'm'
end

local function meta_text(task)
    local meta = format_minutes(task.estimate)
    if meta == '' and task.created ~= today then
        meta = (task.created or ''):sub(6)
    end
    if task.rolled ~= '0' then
        meta = meta .. ' r' .. task.rolled
    end
    return meta
end

local function page_count()
    return math.max(1, math.ceil(#tasks / row_count))
end

local function hide_rows()
    for i = 1, row_count do
        for _, prefix in ipairs({ 'Status', 'PlaceDot', 'PlaceText', 'Title', 'Meta' }) do
            SKIN:Bang('!HideMeter', prefix .. i)
        end
    end
    for _, name in ipairs({ 'RowHover', 'TaskEmpty', 'TaskPrev', 'TaskPage', 'TaskNext', 'TaskBars',
        'TaskScopeDay', 'TaskScopeWeek', 'TaskScopeLong', 'ButtonAdd' }) do
        SKIN:Bang('!HideMeter', name)
    end
end

-- Rows start below the day / week / long toolbar.
local function rows_top()
    return content_top() + TOOLBAR_H
end

local function render_toolbar()
    local y = content_top() + 9
    for name, chip in pairs(SCOPE_CHIP) do
        SKIN:Bang('!SetOption', chip, 'Y', y)
        SKIN:Bang('!SetOption', chip, 'FontColor', color(name == scope and 'ColorAccent' or 'ColorDim'))
        SKIN:Bang('!SetOption', chip, 'SolidColor', color(name == scope and 'ColorSegOn' or 'ColorPanel'))
        SKIN:Bang('!ShowMeter', chip)
    end
    SKIN:Bang('!SetOption', 'ButtonAdd', 'Y', y)
    SKIN:Bang('!SetOption', 'ButtonAdd', 'ToolTipText', 'Add a task to ' .. VIEW_NAME[scope])
    SKIN:Bang('!ShowMeter', 'ButtonAdd')
end

local function render_rows()
    local row_h = num('RowH')
    local top = rows_top()
    local shown = 0
    for i = 1, row_count do
        local task = tasks[(page - 1) * row_count + i]
        local y = top + (i - 1) * row_h
        if task then
            shown = i
            local finished = task.status == 'done' or task.status == 'dropped' or task.status == 'rolled'
            local hit, mark, extra = status_shapes(task.status, row_h)
            SKIN:Bang('!SetOption', 'Status' .. i, 'Y', y)
            SKIN:Bang('!SetOption', 'Status' .. i, 'HalfDisc',
                string.format('8,%g | ArcTo 8,%g,4.5,4.5,0,1 | ClosePath 1', row_h / 2 - 4.5, row_h / 2 + 4.5))
            SKIN:Bang('!SetOption', 'Status' .. i, 'Shape', hit)
            SKIN:Bang('!SetOption', 'Status' .. i, 'Shape2', mark)
            SKIN:Bang('!SetOption', 'Status' .. i, 'Shape3', extra)
            SKIN:Bang('!SetOption', 'PlaceDot' .. i, 'Y', y)
            SKIN:Bang('!SetOption', 'PlaceDot' .. i, 'Shape', place_shape(task.place, row_h))
            SKIN:Bang('!SetOption', 'PlaceText' .. i, 'Y', y + row_h / 2)
            SKIN:Bang('!SetOption', 'PlaceText' .. i, 'Text', PLACE_LETTER[task.place] or '')
            SKIN:Bang('!SetOption', 'Title' .. i, 'Y', y + row_h / 2)
            SKIN:Bang('!SetOption', 'Title' .. i, 'Text', task.title)
            SKIN:Bang('!SetOption', 'Title' .. i, 'FontColor', color(finished and 'ColorDim' or 'ColorText'))
            SKIN:Bang('!SetOption', 'Title' .. i, 'InlineSetting', finished and 'Strikethrough' or '')
            local tooltip = task.title .. ' [' .. task.id .. ', ' .. task.status .. ', ' .. task.place .. ']'
            if task.note ~= '' then
                tooltip = tooltip .. ' -- ' .. task.note
            end
            SKIN:Bang('!SetOption', 'Title' .. i, 'ToolTipText', tooltip .. '. Click: add a note')
            SKIN:Bang('!SetOption', 'Meta' .. i, 'Y', y + row_h / 2)
            SKIN:Bang('!SetOption', 'Meta' .. i, 'Text', meta_text(task))
            SKIN:Bang('!SetOption', 'Meta' .. i, 'ToolTipText',
                'created ' .. task.created .. ', rolled over ' .. task.rolled .. ' times')
            for _, prefix in ipairs({ 'Status', 'PlaceDot', 'PlaceText', 'Title', 'Meta' }) do
                SKIN:Bang('!ShowMeter', prefix .. i)
            end
        else
            for _, prefix in ipairs({ 'Status', 'PlaceDot', 'PlaceText', 'Title', 'Meta' }) do
                SKIN:Bang('!HideMeter', prefix .. i)
            end
        end
    end
    local bottom = top + shown * row_h
    if #tasks == 0 then
        SKIN:Bang('!SetOption', 'TaskEmpty', 'Y', top + row_h / 2)
        SKIN:Bang('!ShowMeter', 'TaskEmpty')
        bottom = top + row_h
    else
        SKIN:Bang('!HideMeter', 'TaskEmpty')
    end
    if page_count() > 1 then
        for _, name in ipairs({ 'TaskPrev', 'TaskPage', 'TaskNext' }) do
            SKIN:Bang('!SetOption', name, 'Y', bottom + 9)
            SKIN:Bang('!ShowMeter', name)
        end
        SKIN:Bang('!SetOption', 'TaskPage', 'Text', page .. '/' .. page_count())
        bottom = bottom + 18
    else
        for _, name in ipairs({ 'TaskPrev', 'TaskPage', 'TaskNext' }) do
            SKIN:Bang('!HideMeter', name)
        end
    end
    SKIN:Bang('!HideMeter', 'RowHover')
    return bottom
end

local function render()
    render_header()
    if view == 'plan' then
        return
    end
    render_sub()
    render_bars()
    SKIN:Bang('!ShowMeter', 'TaskBars')
    render_toolbar()
    local bottom = render_rows()
    SKIN:Bang('!UpdateMeter', '*')
    SKIN:Bang('!CommandMeasure', 'MeasureChat', 'SetTop(' .. math.floor(bottom + 4) .. ')')
end

-- ------------------------------------------------------------ list

function Start()
    SetView(view)
end

-- Every 60 seconds, and after a conversation used tools.
function Tick()
    if input_mode then
        return
    end
    if view == 'plan' then
        render_header()
        SKIN:Bang('!CommandMeasure', 'MeasurePlan', 'Refresh()')
    else
        Reload()
    end
end

function Reload()
    protocol.run('MeasureTaskList', { 'list', scope })
end

function OnListFinished()
    if view == 'plan' then
        return
    end
    local records = parse_records(SKIN:GetMeasure('MeasureTaskList'):GetStringValue())
    tasks = {}
    for _, record in ipairs(records) do
        if record[1] == 'TASK' then
            tasks[#tasks + 1] = {
                id = record[2], status = record[3], title = record[4], created = record[5], rolled = record[6],
                parent = record[7], note = record[8] or '', place = record[9] or 'any', estimate = record[10] or '',
            }
        end
    end
    local summary = summarize(records)
    if summary then
        notify(summary)
    elseif message ~= '' then
        notify(message)
    end
    message = ''
    page = math.min(page, page_count())
    render()
end

function SetView(name)
    if SCOPE_CHIP[name] then
        scope = name
        persist('TaskScope', name)
        name = 'tasks'
    end
    if not TABS[name] then
        return
    end
    view = name
    page = 1
    persist('View', name)
    if name == 'plan' then
        hide_rows()
        render_header()
        SKIN:Bang('!CommandMeasure', 'MeasurePlan', 'Show()')
    else
        SKIN:Bang('!CommandMeasure', 'MeasurePlan', 'Hide()')
        render_header()
        Reload()
    end
end

-- plan_view.lua sets the header subtitle while the Plan tab is shown.
function SetSub(text)
    SKIN:Bang('!SetOption', 'MeterSub', 'Text', text)
    SKIN:Bang('!UpdateMeter', 'MeterSub')
    SKIN:Bang('!UpdateMeter', 'ButtonStyle')
    SKIN:Bang('!Redraw')
end

function SetScope(name)
    if SCOPE_CHIP[name] and view == 'tasks' then
        scope = name
        page = 1
        persist('TaskScope', name)
        Reload()
    end
end

function ChangePage(delta)
    page = math.max(1, math.min(page_count(), page + delta))
    render()
end

function Hover(row)
    if row == 0 or not tasks[(page - 1) * row_count + row] then
        SKIN:Bang('!HideMeter', 'RowHover')
    else
        SKIN:Bang('!SetOption', 'RowHover', 'Y', rows_top() + (row - 1) * num('RowH'))
        SKIN:Bang('!UpdateMeter', 'RowHover')
        SKIN:Bang('!ShowMeter', 'RowHover')
    end
    SKIN:Bang('!Redraw')
end

function CycleStyle()
    persist('Style', NEXT_STYLE[SKIN:GetVariable('Style', 'c')] or 'c')
    SKIN:Bang('!Refresh')
end

-- ------------------------------------------------------------ actions

local function task_at(row)
    return tasks[(page - 1) * row_count + row]
end

local function run_action(args)
    notify('Working...')
    protocol.run('MeasureTaskAction', args)
end

function OnActionFinished()
    local records = parse_records(SKIN:GetMeasure('MeasureTaskAction'):GetStringValue())
    message = summarize(records) or ''
    if message == '' then
        for _, record in ipairs(records) do
            if record[1] == 'COMMIT' then
                message = record[2] == 'none' and 'Nothing to commit' or ('Committed ' .. record[2])
            end
        end
    end
    Reload()
end

function OnStatusClick(row)
    local task = task_at(row)
    if task and NEXT_STATUS[task.status] then
        run_action({ 'status', task.id, NEXT_STATUS[task.status] })
    end
end

function OnStatusAltClick(row)
    local task = task_at(row)
    if task and task.status ~= 'rolled' then
        run_action({ 'status', task.id, ALT_STATUS[task.status] or 'blocked' })
    end
end

function OnPlaceClick(row)
    local task = task_at(row)
    if task and task.status ~= 'rolled' then
        run_action({ 'place', task.id, NEXT_PLACE[task.place] or 'any' })
    end
end

function OnSyncClick()
    run_action({ 'sync' })
end

-- The input window is todo.exe prompt, anchored to this skin's window (titled with the .ini path)
-- at the offset last saved in ui_state.inc.
local function open_input(caption)
    if input_mode then
        return false
    end
    protocol.run('MeasureTaskPrompt', protocol.prompt_colors({
        'prompt', caption,
        '--anchor', SKIN:GetVariable('CURRENTPATH') .. SKIN:GetVariable('CURRENTFILE'),
        '--offset-x', SKIN:GetVariable('TasksPromptX', '0'),
        '--offset-y', SKIN:GetVariable('TasksPromptY', tostring(rows_top())),
    }))
    return true
end

function OnAddClick()
    if view == 'tasks' and open_input('New ' .. VIEW_NAME[scope] .. ' task  (Enter: save, Esc: cancel)') then
        input_mode = 'add'
    end
end

function OnNoteClick(row)
    local task = task_at(row)
    if task and task.status ~= 'rolled' then
        if open_input('Note (Enter: save, Esc: cancel): ' .. task.title) then
            input_mode = 'note'
            input_task_id = task.id
        end
    end
end

function OnPromptFinished()
    local mode = input_mode
    input_mode = nil
    local text = nil
    for _, record in ipairs(parse_records(SKIN:GetMeasure('MeasureTaskPrompt'):GetStringValue())) do
        if record[1] == 'INPUT' then
            text = record[2]
        elseif record[1] == 'POSITION' then
            persist('TasksPromptX', record[2])
            persist('TasksPromptY', record[3])
        elseif record[1] == 'ERR' then
            notify('Error: ' .. (record[2] or ''))
        end
    end
    if text == nil or text:match('^%s*$') then
        return
    end
    if mode == 'add' then
        run_action({ 'add', scope, text })
    elseif mode == 'note' then
        run_action({ 'note', input_task_id, text })
    end
end
