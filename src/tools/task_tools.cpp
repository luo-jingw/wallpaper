#include "tools/task_tools.h"

#include <optional>
#include <string>
#include <utility>

#include "domain/task.h"
#include "domain/task_level.h"
#include "domain/task_place.h"
#include "domain/task_status.h"
#include "domain/task_text.h"
#include "storage/task_file.h"
#include "tools/tool_arguments.h"
#include "util/text.h"
#include "util/todo_error.h"

namespace todo {

namespace {

constexpr char kTaskIdProperty[] = R"("task_id":{"type":"string","description":"Task id such as d20261020-1"})";

TaskLevel parse_level(const std::string& name) {
    const std::optional<TaskLevel> level = TaskLevelCodec::from_name(name);
    if (!level) {
        throw TodoError("level must be one of day, week, long");
    }
    return *level;
}

TaskPlace parse_place(const std::string& name) {
    const std::optional<TaskPlace> place = TaskPlaceCodec::from_name(name);
    if (!place) {
        throw TodoError("place must be one of any, lab, class, remote");
    }
    return *place;
}

TaskLocation require_task(const TaskToolContext& context, const std::string& task_id) {
    std::optional<TaskLocation> location = context.tasks.find(task_id, context.today);
    if (!location) {
        throw TodoError("task not found: " + task_id);
    }
    return std::move(*location);
}

ToolOutcome changed(const Task& task, const std::string& summary) {
    return ToolOutcome{ToolStatus::Ok, "ok: " + Text::trim(TaskText::describe(task)), true, summary};
}

}  // namespace

ToolSpec ListTasksTool::spec() const {
    return ToolSpec{
        "list_tasks",
        "List the user's current tasks. Without level, lists today, this week, and long-term tasks.",
        R"({"type":"object","properties":{"level":{"type":"string","enum":["day","week","long"]}}})",
    };
}

ToolOutcome ListTasksTool::execute(const nlohmann::json& arguments) const {
    const std::optional<std::string> level_name = ToolArguments::optional_string(arguments, "level");
    std::string text;
    int count = 0;
    for (const TaskLevel level : {TaskLevel::Day, TaskLevel::Week, TaskLevel::LongTerm}) {
        if (level_name && parse_level(*level_name) != level) {
            continue;
        }
        const TaskFile file = context_.tasks.load(level, context_.today);
        text += "## " + TaskLevelCodec::to_name(level) + " (" + file.heading + ")\n";
        for (const Task& task : file.tasks) {
            text += TaskText::describe(task);
            ++count;
        }
    }
    return ToolOutcome{ToolStatus::Ok, text, false, level_name.value_or("all") + ", " + std::to_string(count) + " tasks"};
}

ToolSpec AddTaskTool::spec() const {
    return ToolSpec{
        "add_task",
        "Add a task for today (day), this week (week), or long term (long).",
        R"({"type":"object","properties":{)"
        R"("level":{"type":"string","enum":["day","week","long"]},)"
        R"("title":{"type":"string"},)"
        R"("place":{"type":"string","enum":["any","lab","class","remote"]},)"
        R"("estimate_minutes":{"type":"integer","minimum":1}},)"
        R"("required":["level","title"]})",
    };
}

ToolOutcome AddTaskTool::execute(const nlohmann::json& arguments) const {
    const TaskLevel level = parse_level(ToolArguments::required_string(arguments, "level"));
    const std::string title = Text::single_line(ToolArguments::required_string(arguments, "title"));
    if (title.empty()) {
        throw TodoError("title is empty");
    }
    TaskFile file = context_.tasks.load(level, context_.today);
    Task task;
    task.id = context_.tasks.allocate_id(level, context_.today, file);
    task.title = title;
    task.created = context_.today.to_string();
    task.place = parse_place(ToolArguments::optional_string(arguments, "place").value_or("any"));
    task.estimate_minutes = ToolArguments::optional_int(arguments, "estimate_minutes", 1);
    file.tasks.push_back(task);
    context_.tasks.save(level, context_.today, file);
    return changed(task, "added " + task.id + " " + task.title);
}

ToolSpec SetTaskStatusTool::spec() const {
    return ToolSpec{
        "set_task_status",
        "Change a task's status.",
        std::string(R"({"type":"object","properties":{)") + kTaskIdProperty +
            R"(,"status":{"type":"string","enum":["todo","doing","done","blocked","dropped"]}},)"
            R"("required":["task_id","status"]})",
    };
}

ToolOutcome SetTaskStatusTool::execute(const nlohmann::json& arguments) const {
    const std::string task_id = ToolArguments::required_string(arguments, "task_id");
    const std::string name = ToolArguments::required_string(arguments, "status");
    const std::optional<TaskStatus> status = TaskStatusCodec::from_name(name);
    if (!status || *status == TaskStatus::Rolled) {
        throw TodoError("status must be one of todo, doing, done, blocked, dropped");
    }
    TaskLocation location = require_task(context_, task_id);
    Task& task = location.file.tasks[location.index];
    task.status = *status;
    context_.tasks.save(location.level, location.date, location.file);
    return changed(task, task_id + " -> " + name);
}

ToolSpec AddTaskNoteTool::spec() const {
    return ToolSpec{
        "add_task_note",
        "Append a one-line progress note to a task.",
        std::string(R"({"type":"object","properties":{)") + kTaskIdProperty +
            R"(,"text":{"type":"string"}},"required":["task_id","text"]})",
    };
}

ToolOutcome AddTaskNoteTool::execute(const nlohmann::json& arguments) const {
    const std::string task_id = ToolArguments::required_string(arguments, "task_id");
    const std::string text = Text::single_line(ToolArguments::required_string(arguments, "text"));
    if (text.empty()) {
        throw TodoError("note is empty");
    }
    TaskLocation location = require_task(context_, task_id);
    Task& task = location.file.tasks[location.index];
    task.notes.push_back(text);
    context_.tasks.save(location.level, location.date, location.file);
    return changed(task, "note " + task_id);
}

ToolSpec SetTaskPlaceTool::spec() const {
    return ToolSpec{
        "set_task_place",
        "Set where a task happens and/or its estimate in minutes. estimate_minutes 0 clears the estimate.",
        std::string(R"({"type":"object","properties":{)") + kTaskIdProperty +
            R"(,"place":{"type":"string","enum":["any","lab","class","remote"]},)"
            R"("estimate_minutes":{"type":"integer","minimum":0}},"required":["task_id"]})",
    };
}

ToolOutcome SetTaskPlaceTool::execute(const nlohmann::json& arguments) const {
    const std::string task_id = ToolArguments::required_string(arguments, "task_id");
    const std::optional<std::string> place = ToolArguments::optional_string(arguments, "place");
    const std::optional<int> minutes = ToolArguments::optional_int(arguments, "estimate_minutes", 0);
    if (!place && !minutes) {
        throw TodoError("give place, estimate_minutes, or both");
    }
    TaskLocation location = require_task(context_, task_id);
    Task& task = location.file.tasks[location.index];
    if (place) {
        task.place = parse_place(*place);
    }
    if (minutes) {
        task.estimate_minutes = *minutes == 0 ? std::nullopt : minutes;
    }
    context_.tasks.save(location.level, location.date, location.file);
    return changed(task, task_id + " place=" + TaskPlaceCodec::to_name(task.place) + " est=" +
                             (task.estimate_minutes ? std::to_string(*task.estimate_minutes) : "none"));
}

}  // namespace todo
