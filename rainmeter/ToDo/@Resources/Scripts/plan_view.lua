-- The Plan tab of Main.ini: time blocks with task status, suggested changes, apply / discard / generate,
-- and the open day and week tasks the plan does not contain.
-- Plans and tasks are never written here; every change is a todo.exe command.

local protocol   -- todo_protocol.lua, loaded in Initialize()
local PLACE_COLOR = { lab = 'ColorLab', class = 'ColorClass', remote = 'ColorRemote' }
local EMPTY_SHAPE = 'Rectangle 0,0,0,0 | StrokeWidth 0 | Fill Color 0,0,0,0'
local MAX_BARS = 24
local NEXT_STATUS = { todo = 'doing', doing = 'done', done = 'todo', blocked = 'todo', dropped = 'todo' }
local OPEN_STATUS = { todo = true, doing = true, blocked = true }

local block_rows
local change_rows
local visible
local scope
local busy
local plan       -- { period, status, created } or nil
local period     -- the period last shown, also when no plan exists
local blocks
local changes    -- { number, description, applied, selected, title }
local rationale
local page
local show_why
local unplanned_rows
local tasks      -- day and week tasks of the plan period, in file order: { id, status, title, place, level }
local task_by_id
local unplanned  -- open tasks no block refers to

function Initialize()
    protocol = dofile(SKIN:GetVariable('@') .. 'Scripts\\todo_protocol.lua')
    block_rows = SELF:GetNumberOption('BlockRows', 12)
    change_rows = SELF:GetNumberOption('ChangeRows', 8)
    unplanned_rows = SELF:GetNumberOption('UnplannedRows', 6)
    tasks = {}
    task_by_id = {}
    unplanned = {}
    visible = false
    scope = 'day'
    busy = false
    blocks = {}
    changes = {}
    rationale = ''
    page = 1
    show_why = false
end

function Update()
    return #blocks
end

-- ------------------------------------------------------------ protocol

-- todo.exe output for this view: line breaks kept, TABs as one space.
local function parse_records(output)
    return protocol.parse_records(output, { newline = '\n', tab = ' ' })
end

local function num(name)
    return tonumber(SKIN:GetVariable(name)) or 0
end

local function color(name)
    return SKIN:GetVariable(name)
end

local function bracket_safe(text)
    return ((text or ''):gsub('%]%]', '] ]'):gsub('[\r\n]', ' '))
end

-- chat_view.lua owns the message line.
local function notify(text)
    SKIN:Bang('!CommandMeasure', 'MeasureChat', 'ShowMessage([[' .. bracket_safe(text) .. ']])')
end

-- ------------------------------------------------------------ helpers

-- Same formula as task_view.lua.
local function content_top()
    return num('PadY') + num('HeaderH') + 4 + num('BarH') + 8
end

local function clock_minutes(text)
    local h, m = (text or ''):match('(%d+):(%d+)$')
    if not h then
        return nil
    end
    return tonumber(h) * 60 + tonumber(m)
end

local function format_minutes(minutes)
    if not minutes or minutes <= 0 then
        return ''
    elseif minutes < 60 then
        return minutes .. 'm'
    elseif minutes % 60 == 0 then
        return (minutes / 60) .. 'h'
    end
    return math.floor(minutes / 60) .. 'h' .. (minutes % 60) .. 'm'
end

local function block_minutes(block)
    local a, b = clock_minutes(block.start), clock_minutes(block['end'])
    return (a and b) and (b - a) or nil
end

-- Per day: switches between remote and campus (lab, class) across non-commute blocks, and commute minutes.
local function block_stats()
    local switches, commute = 0, 0
    local side, day = nil, nil
    for _, block in ipairs(blocks) do
        local block_day = block.start:match('^(%a+) ') or ''
        if block_day ~= day then
            day, side = block_day, nil
        end
        if block.kind == 'commute' then
            commute = commute + (block_minutes(block) or 0)
        else
            local current = (block.place == 'lab' or block.place == 'class') and 'campus'
                or (block.place == 'remote' and 'remote' or nil)
            if current then
                if side and side ~= current then
                    switches = switches + 1
                end
                side = current
            end
        end
    end
    return switches, commute
end

local function change_text(change)
    local words = {}
    for word in change.description:gmatch('%S+') do
        words[#words + 1] = word
    end
    local op = words[1]
    if op == 'set_place' then
        return 'place  ' .. (change.title ~= '' and change.title or words[2]) .. '  -> ' .. (words[3] or '')
    elseif op == 'set_estimate' then
        return 'estimate  ' .. (change.title ~= '' and change.title or words[2]) .. '  -> ' ..
            format_minutes(tonumber(words[3]))
    elseif op == 'add_task' then
        local title = change.description:match('^add_task%s+%S+%s+%S+%s+%S+%s+(.*)$') or ''
        local estimate = words[4] ~= '-' and ('  ' .. format_minutes(tonumber(words[4]))) or ''
        return 'add ' .. (words[2] or '') .. '  ' .. title .. '  ' .. (words[3] or '') .. estimate
    end
    return change.description
end

local function selected_numbers()
    local numbers = {}
    for _, change in ipairs(changes) do
        if change.selected and not change.applied then
            numbers[#numbers + 1] = change.number
        end
    end
    return numbers
end

-- ------------------------------------------------------------ rendering

local function hide_all()
    SKIN:Bang('!HideMeterGroup', 'Plan')
end

local function set_row_y(meter, y)
    SKIN:Bang('!SetOption', meter, 'Y', y)
    SKIN:Bang('!ShowMeter', meter)
end

local function render_bars()
    local width = num('SkinWidth') - 2 * num('PadX')
    local gap = num('BarGap')
    local height = num('BarH')
    local shown = {}
    for _, block in ipairs(blocks) do
        if block.kind ~= 'break' and #shown < MAX_BARS then
            shown[#shown + 1] = block
        end
    end
    local segment = #shown > 0 and (width - gap * (#shown - 1)) / #shown or 0
    for i = 1, MAX_BARS do
        local block = shown[i]
        local value = EMPTY_SHAPE
        if block then
            local fill = block.kind == 'commute' and 'ColorLine' or (PLACE_COLOR[block.place] or 'ColorAccent')
            value = string.format('Rectangle %.1f,0,%.1f,%d,%.1f | Fill Color %s | StrokeWidth 0',
                (i - 1) * (segment + gap), segment, height, height / 2, color(fill))
        end
        SKIN:Bang('!SetOption', 'PlanBars', i == 1 and 'Shape' or ('Shape' .. i), value)
    end
    SKIN:Bang('!ShowMeter', 'PlanBars')
end

local function node_shape(block, height)
    local cy = height / 2
    if block.kind == 'commute' then
        return string.format('Ellipse 6,%g,4.5 | StrokeWidth 1.2 | Stroke Color %s | StrokeDashes 2,1.5 | Fill Color 0,0,0,0',
            cy, color('ColorDim'))
    elseif block.kind == 'break' then
        return string.format('Ellipse 6,%g,2 | StrokeWidth 0 | Fill Color %s', cy, color('ColorDim'))
    elseif block.kind == 'fixed' then
        return string.format('Rectangle 2,%g,8,8,2 | StrokeWidth 0 | Fill Color %s', cy - 4,
            color(PLACE_COLOR[block.place] or 'ColorAccent'))
    end
    return string.format('Ellipse 6,%g,4 | StrokeWidth 0 | Fill Color %s', cy, color(PLACE_COLOR[block.place] or 'ColorDim'))
end

local function status_shapes(status, place, height)
    local cx, cy, r = 6, height / 2, 4.5
    local ring_color = color(PLACE_COLOR[place] or 'ColorDim')
    local hit = 'Rectangle -2,0,18,' .. height .. ' | StrokeWidth 0 | Fill Color 0,0,0,1'
    local function ring(value)
        return string.format('Ellipse %g,%g,%g | StrokeWidth 1.3 | Stroke Color %s | Fill Color 0,0,0,0', cx, cy, r, value)
    end
    if status == 'done' then
        return hit, string.format('Ellipse %g,%g,3.5 | StrokeWidth 0 | Fill Color %s', cx, cy, color('ColorDone')), EMPTY_SHAPE
    elseif status == 'doing' then
        return hit, ring(ring_color), 'Path HalfDisc | StrokeWidth 0 | Fill Color ' .. ring_color
    elseif status == 'blocked' or status == 'dropped' then
        local value = color(status == 'blocked' and 'ColorBlocked' or 'ColorDim')
        return hit, ring(value), string.format('Line %g,%g,%g,%g | StrokeWidth 1.3 | Stroke Color %s',
            cx - 3.2, cy + 3.2, cx + 3.2, cy - 3.2, value)
    end
    return hit, ring(ring_color), EMPTY_SHAPE
end

-- Draws a task's status into a StyleRowShape meter; the meter's HalfDisc path is set for the doing state.
local function set_status_mark(meter, status, place, height)
    local hit, mark, extra = status_shapes(status, place, height)
    SKIN:Bang('!SetOption', meter, 'HalfDisc',
        string.format('6,%g | ArcTo 6,%g,4.5,4.5,0,1 | ClosePath 1', height / 2 - 4.5, height / 2 + 4.5))
    SKIN:Bang('!SetOption', meter, 'Shape', hit)
    SKIN:Bang('!SetOption', meter, 'Shape2', mark)
    SKIN:Bang('!SetOption', meter, 'Shape3', extra)
end

local function compute_unplanned()
    local referenced = {}
    for _, block in ipairs(blocks) do
        if block.task_id ~= '' then
            referenced[block.task_id] = true
        end
    end
    unplanned = {}
    for _, task in ipairs(tasks) do
        if OPEN_STATUS[task.status] and not referenced[task.id] then
            unplanned[#unplanned + 1] = task
        end
    end
end

-- Returns the y below the list.
local function render_unplanned(y, has_plan)
    if #unplanned == 0 then
        return y
    end
    local row_h = num('ChangeRowH')
    y = y + 6
    local label = has_plan and 'NOT IN PLAN' or 'OPEN TASKS'
    if #unplanned > unplanned_rows then
        label = label .. '  (+' .. (#unplanned - unplanned_rows) .. ' more in Tasks)'
    end
    set_row_y('UnplannedLabel', y + 9)
    SKIN:Bang('!SetOption', 'UnplannedLabel', 'Text', label)
    y = y + 20
    for i = 1, math.min(#unplanned, unplanned_rows) do
        local task = unplanned[i]
        local row_y = y + (i - 1) * row_h
        set_row_y('UnplannedMark' .. i, row_y)
        set_status_mark('UnplannedMark' .. i, task.status, task.place, row_h)
        set_row_y('UnplannedText' .. i, row_y + row_h / 2)
        SKIN:Bang('!SetOption', 'UnplannedText' .. i, 'Text', task.title)
        SKIN:Bang('!SetOption', 'UnplannedText' .. i, 'ToolTipText', task.title .. ' [' .. task.id .. ', ' .. task.status .. ']')
        set_row_y('UnplannedMeta' .. i, row_y + row_h / 2)
        SKIN:Bang('!SetOption', 'UnplannedMeta' .. i, 'Text',
            (task.level == 'day' and 'today' or 'week') .. (task.place ~= 'any' and ('  ' .. task.place) or ''))
    end
    return y + math.min(#unplanned, unplanned_rows) * row_h
end

local function page_count()
    return math.max(1, math.ceil(#blocks / block_rows))
end

local function render()
    if not visible then
        return
    end
    hide_all()
    local top = content_top()
    local tl_h = num('TlRowH')
    local change_h = num('ChangeRowH')

    -- Toolbar: scope chips, state, generate.
    local y = top + 9
    for _, name in ipairs({ 'PlanScopeDay', 'PlanScopeWeek', 'PlanState', 'PlanGenerate' }) do
        set_row_y(name, y)
    end
    SKIN:Bang('!SetOption', 'PlanScopeDay', 'FontColor', color(scope == 'day' and 'ColorAccent' or 'ColorDim'))
    SKIN:Bang('!SetOption', 'PlanScopeDay', 'SolidColor', color(scope == 'day' and 'ColorSegOn' or 'ColorPanel'))
    SKIN:Bang('!SetOption', 'PlanScopeWeek', 'FontColor', color(scope == 'week' and 'ColorAccent' or 'ColorDim'))
    SKIN:Bang('!SetOption', 'PlanScopeWeek', 'SolidColor', color(scope == 'week' and 'ColorSegOn' or 'ColorPanel'))
    local state = busy and 'working...' or ''
    if not busy and plan then
        state = plan.status .. '  ' .. (plan.created:match('T(.*)$') or plan.created)
    end
    SKIN:Bang('!SetOption', 'PlanState', 'Text', state)
    SKIN:Bang('!SetOption', 'PlanGenerate', 'Text', plan and 're-plan' or 'plan')
    y = top + 22

    local sub = scope .. ' plan'
    if not plan then
        set_row_y('PlanEmpty', y + 11)
        SKIN:Bang('!SetOption', 'PlanEmpty', 'Text',
            'No ' .. scope .. ' plan for ' .. (period or 'this ' .. scope) .. '. Click plan to make one.')
        y = y + 22
        y = render_unplanned(y, false)
    else
        local switches, commute = block_stats()
        sub = 'commute ' .. (commute > 0 and format_minutes(commute) or '0m') ..
            (switches > 0 and (', ' .. switches .. ' sw') or '')
        if plan.status == 'discarded' then
            sub = sub .. ', discarded'
        end
        render_bars()
        y = y + 4
        local shown = 0
        for i = 1, block_rows do
            local block = blocks[(page - 1) * block_rows + i]
            if block then
                shown = i
                local row_y = y + (i - 1) * tl_h
                local task = task_by_id[block.task_id]
                local finished = task and (task.status == 'done' or task.status == 'dropped')
                local dim = block.kind == 'commute' or block.kind == 'break' or finished
                set_row_y('TlTime' .. i, row_y + tl_h / 2)
                SKIN:Bang('!SetOption', 'TlTime' .. i, 'Text', block.start)
                set_row_y('TlNode' .. i, row_y)
                if task then
                    -- A block that works on a known task shows that task's status; click to advance it.
                    set_status_mark('TlNode' .. i, task.status, block.place, tl_h)
                    SKIN:Bang('!SetOption', 'TlNode' .. i, 'ToolTipText', task.status .. '. Click: todo > doing > done')
                else
                    SKIN:Bang('!SetOption', 'TlNode' .. i, 'Shape', node_shape(block, tl_h))
                    SKIN:Bang('!SetOption', 'TlNode' .. i, 'Shape2', EMPTY_SHAPE)
                    SKIN:Bang('!SetOption', 'TlNode' .. i, 'Shape3', EMPTY_SHAPE)
                    SKIN:Bang('!SetOption', 'TlNode' .. i, 'ToolTipText', '')
                end
                set_row_y('TlTitle' .. i, row_y + tl_h / 2)
                SKIN:Bang('!SetOption', 'TlTitle' .. i, 'Text', block.title)
                SKIN:Bang('!SetOption', 'TlTitle' .. i, 'FontColor', color(dim and 'ColorDim' or 'ColorText'))
                SKIN:Bang('!SetOption', 'TlTitle' .. i, 'InlineSetting',
                    finished and 'Strikethrough' or (block.kind == 'commute' and 'Italic' or ''))
                SKIN:Bang('!SetOption', 'TlTitle' .. i, 'ToolTipText',
                    block.start .. '-' .. block['end'] .. '  ' .. block.kind .. '  ' .. block.place ..
                    (block.task_id ~= '' and ('  ' .. block.task_id) or ''))
                set_row_y('TlDur' .. i, row_y + tl_h / 2)
                SKIN:Bang('!SetOption', 'TlDur' .. i, 'Text', format_minutes(block_minutes(block)))
            end
        end
        y = y + shown * tl_h
        if page_count() > 1 then
            for _, name in ipairs({ 'TlPrev', 'TlPage', 'TlNext' }) do
                set_row_y(name, y + 9)
            end
            SKIN:Bang('!SetOption', 'TlPage', 'Text', page .. '/' .. page_count())
            y = y + 18
        end

        -- Changes.
        y = y + 6
        set_row_y('ChangesLabel', y + 9)
        set_row_y('ButtonWhy', y + 9)
        SKIN:Bang('!SetOption', 'ChangesLabel', 'Text', #changes > 0 and 'CHANGES' or 'NO CHANGES')
        y = y + 20
        if show_why and rationale ~= '' then
            set_row_y('RationaleText', y)
            SKIN:Bang('!SetOption', 'RationaleText', 'Text', rationale)
            SKIN:Bang('!UpdateMeter', 'RationaleText')
            y = y + SKIN:GetMeter('RationaleText'):GetH() + 4
        end
        for i = 1, math.min(#changes, change_rows) do
            local change = changes[i]
            local row_y = y + (i - 1) * change_h
            local cy = change_h / 2
            local box
            if change.applied then
                box = string.format('Rectangle 1,%g,11,11,2 | StrokeWidth 0 | Fill Color %s', cy - 5.5, color('ColorDone'))
            elseif change.selected then
                box = string.format('Rectangle 1,%g,11,11,2 | StrokeWidth 1.2 | Stroke Color %s | Fill Color %s',
                    cy - 5.5, color('ColorAccent'), color('ColorSegOn'))
            else
                box = string.format('Rectangle 1,%g,11,11,2 | StrokeWidth 1.2 | Stroke Color %s | Fill Color 0,0,0,0',
                    cy - 5.5, color('ColorDim'))
            end
            set_row_y('ChangeBox' .. i, row_y)
            SKIN:Bang('!SetOption', 'ChangeBox' .. i, 'Shape',
                'Rectangle 0,0,16,' .. change_h .. ' | StrokeWidth 0 | Fill Color 0,0,0,1')
            SKIN:Bang('!SetOption', 'ChangeBox' .. i, 'Shape2', box)
            SKIN:Bang('!SetOption', 'ChangeBox' .. i, 'Shape3',
                change.selected and not change.applied
                and string.format('Rectangle 4,%g,5,5,1 | StrokeWidth 0 | Fill Color %s', cy - 2.5, color('ColorAccent'))
                or EMPTY_SHAPE)
            set_row_y('ChangeText' .. i, row_y + cy)
            SKIN:Bang('!SetOption', 'ChangeText' .. i, 'Text',
                change_text(change) .. (change.applied and '  (applied)' or ''))
            SKIN:Bang('!SetOption', 'ChangeText' .. i, 'FontColor', color(change.applied and 'ColorDim' or 'ColorText'))
            SKIN:Bang('!SetOption', 'ChangeText' .. i, 'ToolTipText', '[' .. change.number .. '] ' .. change.description)
        end
        y = y + math.min(#changes, change_rows) * change_h
        if plan.status == 'proposed' then
            y = y + 4
            set_row_y('ButtonDiscard', y + 9)
            set_row_y('ButtonApply', y + 9)
            local count = #selected_numbers()
            SKIN:Bang('!SetOption', 'ButtonApply', 'Text', 'apply ' .. count)
            SKIN:Bang('!SetOption', 'ButtonApply', 'ToolTipText', 'Apply the checked changes to the tasks')
            y = y + 20
        end
        y = render_unplanned(y, true)
    end
    SKIN:Bang('!CommandMeasure', 'MeasureTasks', 'SetSub([[' .. bracket_safe(sub) .. ']])')
    SKIN:Bang('!UpdateMeterGroup', 'Plan')
    SKIN:Bang('!CommandMeasure', 'MeasureChat', 'SetTop(' .. math.floor(y + 4) .. ')')
end

-- ------------------------------------------------------------ commands

local function start(args, text)
    if busy then
        return
    end
    busy = true
    notify(text)
    render()
    protocol.run('MeasurePlanRun', args)
end

function Show()
    visible = true
    page = 1
    start({ 'plan', 'show', scope }, '')
end

function Hide()
    visible = false
    hide_all()
    SKIN:Bang('!Redraw')
end

function SetScope(name)
    if busy or name == scope then
        return
    end
    scope = name
    page = 1
    start({ 'plan', 'show', scope }, '')
end

-- The current conversation goes into the request, so what the user said there (for example when they
-- leave home) shapes the plan.
function Generate()
    local args = { 'plan', scope }
    local chat = SKIN:GetVariable('CurrentChat', '')
    if chat ~= '' then
        args[#args + 1] = '--chat'
        args[#args + 1] = chat
    end
    start(args, chat ~= '' and 'Planning with DeepSeek (with the current conversation)...' or 'Planning with DeepSeek...')
    -- The mascot (ToDo\Mascot) writes in her notebook until the plan arrives.
    SKIN:Bang('!CommandMeasure', 'MeasureMascot', 'StartWriting()', 'ToDo\\Mascot')
end

-- chat_view.lua calls this after propose_plan stored a plan; task_view.lua then shows the Plan tab.
function Focus(name)
    if name == 'day' or name == 'week' then
        scope = name
    end
    page = 1
end

function Apply()
    if not plan or plan.status ~= 'proposed' then
        return
    end
    local selection = {}
    for _, number in ipairs(selected_numbers()) do
        selection[#selection + 1] = tostring(number)
    end
    if #selection == 0 then
        notify('Check at least one change to apply, or discard the plan.')
        return
    end
    start({ 'plan', 'apply', plan.period, table.concat(selection, ',') }, 'Applying...')
end

function Discard()
    if plan and plan.status == 'proposed' then
        start({ 'plan', 'discard', plan.period }, 'Discarding...')
    end
end

function ToggleChange(row)
    local change = changes[row]
    if change and not change.applied and plan and plan.status == 'proposed' then
        change.selected = not change.selected
        render()
        SKIN:Bang('!Redraw')
    end
end

function ToggleWhy()
    show_why = not show_why
    render()
    SKIN:Bang('!Redraw')
end

function ChangePage(delta)
    page = math.max(1, math.min(page_count(), page + delta))
    render()
    SKIN:Bang('!Redraw')
end

function OnRunFinished()
    busy = false
    SKIN:Bang('!CommandMeasure', 'MeasureMascot', 'StopWriting()', 'ToDo\\Mascot')
    local records = parse_records(SKIN:GetMeasure('MeasurePlanRun'):GetStringValue())
    local error_text = nil
    local usage_text = ''
    local loaded = nil
    local loaded_blocks, loaded_changes, loaded_rationale = {}, {}, ''
    local missing = nil
    local loaded_tasks = {}
    for _, record in ipairs(records) do
        local kind = record[1]
        if kind == 'TASK' then
            loaded_tasks[#loaded_tasks + 1] = {
                id = record[2], status = record[3], title = record[4] or '', place = record[9] or 'any',
                level = (record[2] or ''):sub(1, 1) == 'w' and 'week' or 'day',
            }
        end
        if kind == 'PLAN' then
            loaded = { period = record[2], status = record[3], created = record[4] or '' }
        elseif kind == 'BLOCK' then
            loaded_blocks[#loaded_blocks + 1] = {
                start = record[2], ['end'] = record[3], kind = record[4], place = record[5], title = record[6] or '',
                task_id = record[7] or '',
            }
        elseif kind == 'CHANGE' then
            local applied = record[4] == 'applied'
            loaded_changes[#loaded_changes + 1] = {
                number = tonumber(record[2]), description = record[3] or '', applied = applied, selected = true,
                title = record[5] or '',
            }
        elseif kind == 'RATIONALE' then
            loaded_rationale = record[2] or ''
        elseif kind == 'USAGE' then
            usage_text = 'in ' .. record[2] .. ' / out ' .. record[3] .. ' tokens, ' .. record[4] .. ' ms'
        elseif kind == 'OK' then
            missing = (record[2] or ''):match('^no plan for (.*)$')
        elseif kind == 'ERR' then
            error_text = 'Error: ' .. (record[2] or '')
        end
    end
    if error_text then
        notify(error_text)
    else
        tasks = loaded_tasks
        task_by_id = {}
        for _, task in ipairs(tasks) do
            task_by_id[task.id] = task
        end
        if loaded then
            for _, change in ipairs(loaded_changes) do
                change.selected = loaded.status == 'proposed'
            end
            plan, period = loaded, loaded.period
            blocks, changes, rationale = loaded_blocks, loaded_changes, loaded_rationale
        elseif missing then
            plan, period = nil, missing
            blocks, changes, rationale = {}, {}, ''
        end
        -- Only a new plan reports usage; a refresh leaves the message line alone.
        if usage_text ~= '' then
            notify(usage_text)
        end
    end
    compute_unplanned()
    page = math.min(page, page_count())
    render()
    SKIN:Bang('!Redraw')
end

-- ------------------------------------------------------------ task status from the Plan tab

local function advance(task)
    if task and NEXT_STATUS[task.status] and not busy then
        busy = true
        protocol.run('MeasurePlanAction', { 'status', task.id, NEXT_STATUS[task.status] })
    end
end

function OnBlockStatusClick(row)
    local block = blocks[(page - 1) * block_rows + row]
    if block then
        advance(task_by_id[block.task_id])
    end
end

function OnUnplannedClick(row)
    advance(unplanned[row])
end

function OnActionFinished()
    busy = false
    for _, record in ipairs(parse_records(SKIN:GetMeasure('MeasurePlanAction'):GetStringValue())) do
        if record[1] == 'ERR' then
            notify('Error: ' .. (record[2] or ''))
        end
    end
    Refresh()
end

-- Reloads the shown plan and task states without touching the message line.
function Refresh()
    if visible and not busy then
        busy = true
        protocol.run('MeasurePlanRun', { 'plan', 'show', scope })
    end
end
