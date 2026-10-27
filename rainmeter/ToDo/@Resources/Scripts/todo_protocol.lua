-- The todo.exe protocol shared by the view scripts (task_view, plan_view, chat_view, companion_view).
-- A Script measure has one ScriptFile, so each view loads this file in Initialize():
--   protocol = dofile(SKIN:GetVariable('@') .. 'Scripts\\todo_protocol.lua')
-- Interface (the returned table):
--   parse_records(output, format) -> { { type, field, ... }, ... }
--       output: todo.exe's stdout, one TAB-separated record per line, fields escaped by SkinOutput.
--       format: { newline = text for an escaped newline, tab = text for an escaped TAB }.
--   quote(arg) -> arg in double quotes, safe on a Windows command line.
--   run(measure, args): runs todo.exe through a RunCommand measure with --config and the quoted args.
--   prompt_colors(args) -> args with the todo.exe prompt color options of the current style appended.

local protocol = {}

local function unescape(field, format)
    return (field:gsub('\\(.)', function(c)
        if c == 'n' then
            return format.newline
        elseif c == 't' then
            return format.tab
        end
        return c
    end))
end

function protocol.parse_records(output, format)
    local records = {}
    for line in output:gmatch('[^\r\n]+') do
        local fields = {}
        for field in (line .. '\t'):gmatch('([^\t]*)\t') do
            fields[#fields + 1] = unescape(field, format)
        end
        records[#records + 1] = fields
    end
    return records
end

-- Embedded quotes and trailing backslashes would break the Windows command line, so they are replaced or
-- removed.
function protocol.quote(arg)
    local cleaned = arg:gsub('"', "'"):gsub('\\+$', '')
    return '"' .. cleaned .. '"'
end

function protocol.run(measure, args)
    local parts = { '--config', protocol.quote(SKIN:GetVariable('TodoConfig')) }
    for _, arg in ipairs(args) do
        parts[#parts + 1] = protocol.quote(arg)
    end
    SKIN:Bang('!SetOption', measure, 'Parameter', table.concat(parts, ' '))
    SKIN:Bang('!UpdateMeasure', measure)
    SKIN:Bang('!CommandMeasure', measure, 'Run')
end

-- The input window in the current style's colors (style-{a,b,c}.inc).
function protocol.prompt_colors(args)
    for _, pair in ipairs({ { '--color-bg', 'ColorBg' }, { '--color-panel', 'ColorPanel' },
        { '--color-text', 'ColorText' }, { '--color-dim', 'ColorDim' }, { '--color-accent', 'ColorAccent' } }) do
        args[#args + 1] = pair[1]
        args[#args + 1] = SKIN:GetVariable(pair[2])
    end
    return args
end

return protocol
