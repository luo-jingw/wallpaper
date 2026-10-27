#include "cli/task_commands.h"

#include <optional>
#include <string>

#include "domain/task.h"
#include "domain/task_level.h"
#include "domain/task_place.h"
#include "domain/task_status.h"
#include "storage/task_file.h"
#include "util/text.h"
#include "util/todo_error.h"

namespace todo {

void TaskCommands::list(const Command& command) const {
    const TaskLevel level = context_.require_level(command, 0);
    const TaskFile file = context_.tasks.load(level, command.date.value_or(context_.today));
    for (const Task& task : file.tasks) {
        context_.output.task(task);
    }
    context_.output.ok(file.heading);
}

void TaskCommands::add(const Command& command) const {
    const TaskLevel level = context_.require_level(command, 0);
    const std::string title = Text::single_line(context_.require_positional(command, 1, "title"));
    if (title.empty()) {
        throw TodoError("title is empty");
    }
    if (command.parent) {
        const std::optional<TaskLocation> parent = context_.tasks.find(*command.parent, context_.today);
        if (!parent) {
            throw TodoError("parent not found: " + *command.parent);
        }
        const bool valid = (level == TaskLevel::Day && parent->level != TaskLevel::Day) ||
                           (level == TaskLevel::Week && parent->level == TaskLevel::LongTerm);
        if (!valid) {
            throw TodoError("a " + TaskLevelCodec::to_name(level) + " task cannot have a " +
                            TaskLevelCodec::to_name(parent->level) + " parent");
        }
    }

    TaskFile file = context_.tasks.load(level, context_.today);
    Task task;
    task.id = context_.tasks.allocate_id(level, context_.today, file);
    task.title = title;
    task.created = context_.today.to_string();
    task.parent = command.parent;
    file.tasks.push_back(task);
    context_.tasks.save(level, context_.today, file);
    context_.output.task(task);
    context_.commit("add " + task.id + ": " + task.title);
    context_.output.ok("added " + task.id);
}

void TaskCommands::status(const Command& command) const {
    const std::string& task_id = context_.require_positional(command, 0, "task_id");
    const std::string& name = context_.require_positional(command, 1, "status");
    const std::optional<TaskStatus> status = TaskStatusCodec::from_name(name);
    if (!status || *status == TaskStatus::Rolled) {
        throw TodoError("status must be one of todo, doing, done, blocked, dropped");
    }
    std::optional<TaskLocation> location = context_.tasks.find(task_id, context_.today);
    if (!location) {
        throw TodoError("task not found: " + task_id);
    }
    Task& task = location->file.tasks[location->index];
    task.status = *status;
    context_.tasks.save(location->level, location->date, location->file);
    context_.output.task(task);
    context_.commit("status " + task_id + " " + name + ": " + task.title);
    context_.output.ok(task_id + " -> " + name);
}

void TaskCommands::note(const Command& command) const {
    const std::string& task_id = context_.require_positional(command, 0, "task_id");
    const std::string text = Text::single_line(context_.require_positional(command, 1, "text"));
    if (text.empty()) {
        throw TodoError("note is empty");
    }
    std::optional<TaskLocation> location = context_.tasks.find(task_id, context_.today);
    if (!location) {
        throw TodoError("task not found: " + task_id);
    }
    Task& task = location->file.tasks[location->index];
    task.notes.push_back(text);
    context_.tasks.save(location->level, location->date, location->file);
    context_.output.task(task);
    context_.commit("note " + task_id + ": " + text);
    context_.output.ok("noted " + task_id);
}

void TaskCommands::place(const Command& command) const {
    const std::string& task_id = context_.require_positional(command, 0, "task_id");
    const std::string& name = context_.require_positional(command, 1, "place");
    const std::optional<TaskPlace> place = TaskPlaceCodec::from_name(name);
    if (!place) {
        throw TodoError("place must be one of any, lab, class, remote");
    }
    std::optional<TaskLocation> location = context_.tasks.find(task_id, context_.today);
    if (!location) {
        throw TodoError("task not found: " + task_id);
    }
    Task& task = location->file.tasks[location->index];
    task.place = *place;
    context_.tasks.save(location->level, location->date, location->file);
    context_.output.task(task);
    context_.commit("place " + task_id + " " + name + ": " + task.title);
    context_.output.ok(task_id + " -> " + name);
}

void TaskCommands::estimate(const Command& command) const {
    const std::string& task_id = context_.require_positional(command, 0, "task_id");
    const std::string& value = context_.require_positional(command, 1, "minutes");
    std::optional<int> minutes;
    if (value != "none") {
        minutes = Text::parse_positive_int(value);
        if (!minutes) {
            throw TodoError("estimate must be a positive number of minutes or none, got: " + value);
        }
    }
    std::optional<TaskLocation> location = context_.tasks.find(task_id, context_.today);
    if (!location) {
        throw TodoError("task not found: " + task_id);
    }
    Task& task = location->file.tasks[location->index];
    task.estimate_minutes = minutes;
    context_.tasks.save(location->level, location->date, location->file);
    context_.output.task(task);
    context_.commit("estimate " + task_id + " " + value + ": " + task.title);
    context_.output.ok(task_id + " -> " + value);
}

void TaskCommands::sync() const {
    context_.commit("sync manual edits");
    context_.output.ok("synced");
}

}  // namespace todo
