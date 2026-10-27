#include "storage/task_file.h"

#include <optional>

#include "domain/task_place.h"
#include "domain/task_status.h"
#include "util/text.h"
#include "util/todo_error.h"

namespace todo {

namespace {

const std::string kMetaOpen = "<!--";
const std::string kMetaClose = "-->";
const std::string kNotePrefix = "  > ";

void apply_metadata(Task& task, const std::string& metadata, const std::string& location) {
    for (const std::string& pair : Text::split_whitespace(metadata)) {
        const auto equals = pair.find('=');
        if (equals == std::string::npos) {
            throw TodoError(location + ": invalid metadata entry: " + pair);
        }
        const std::string key = pair.substr(0, equals);
        const std::string value = pair.substr(equals + 1);
        if (key == "id") {
            task.id = value;
        } else if (key == "created") {
            task.created = value;
        } else if (key == "rolled") {
            task.rolled = std::stoi(value);
        } else if (key == "parent") {
            task.parent = value;
        } else if (key == "place") {
            const std::optional<TaskPlace> place = TaskPlaceCodec::from_name(value);
            if (!place) {
                throw TodoError(location + ": unknown place: " + value);
            }
            task.place = *place;
        } else if (key == "est") {
            const std::optional<int> minutes = Text::parse_positive_int(value);
            if (!minutes) {
                throw TodoError(location + ": estimate must be a positive integer: " + value);
            }
            task.estimate_minutes = *minutes;
        } else {
            throw TodoError(location + ": unknown metadata key: " + key);
        }
    }
}

std::optional<Task> parse_task_line(const std::string& line, const std::string& location) {
    if (line.size() < 6 || !Text::starts_with(line, "- [") || line[4] != ']' || line[5] != ' ') {
        return std::nullopt;
    }
    const std::optional<TaskStatus> status = TaskStatusCodec::from_marker(line[3]);
    if (!status) {
        throw TodoError(location + ": unknown status marker: " + line.substr(3, 1));
    }
    Task task;
    task.status = *status;
    std::string rest = line.substr(6);
    const auto open = rest.rfind(kMetaOpen);
    if (open != std::string::npos) {
        const auto close = rest.find(kMetaClose, open);
        if (close == std::string::npos) {
            throw TodoError(location + ": unterminated metadata comment");
        }
        apply_metadata(task, rest.substr(open + kMetaOpen.size(), close - open - kMetaOpen.size()), location);
        rest = rest.substr(0, open);
    }
    task.title = Text::trim(rest);
    if (task.title.empty()) {
        throw TodoError(location + ": task without title");
    }
    return task;
}

}  // namespace

TaskFile TaskFileCodec::parse(const std::string& utf8_text, const std::string& source_name) const {
    TaskFile file;
    const std::vector<std::string> lines = Text::split_lines(utf8_text);
    for (size_t i = 0; i < lines.size(); ++i) {
        const std::string& line = lines[i];
        const std::string location = source_name + ":" + std::to_string(i + 1);
        if (Text::trim(line).empty()) {
            continue;
        }
        if (Text::starts_with(line, "# ") && file.heading.empty() && file.tasks.empty()) {
            file.heading = Text::trim(line.substr(2));
            continue;
        }
        if (Text::starts_with(line, kNotePrefix)) {
            if (file.tasks.empty()) {
                throw TodoError(location + ": note before any task");
            }
            file.tasks.back().notes.push_back(Text::trim(line.substr(kNotePrefix.size())));
            continue;
        }
        std::optional<Task> task = parse_task_line(line, location);
        if (!task) {
            throw TodoError(location + ": unrecognized line: " + line);
        }
        file.tasks.push_back(std::move(*task));
    }
    return file;
}

std::string TaskFileCodec::serialize(const TaskFile& file) const {
    std::string out = "# " + file.heading + "\n\n";
    for (const Task& task : file.tasks) {
        out += "- [";
        out += TaskStatusCodec::to_marker(task.status);
        out += "] " + task.title + " " + kMetaOpen + " id=" + task.id + " created=" + task.created +
               " rolled=" + std::to_string(task.rolled);
        if (task.parent) {
            out += " parent=" + *task.parent;
        }
        if (task.place != TaskPlace::Any) {
            out += " place=" + TaskPlaceCodec::to_name(task.place);
        }
        if (task.estimate_minutes) {
            out += " est=" + std::to_string(*task.estimate_minutes);
        }
        out += " " + kMetaClose + "\n";
        for (const std::string& note : task.notes) {
            out += kNotePrefix + note + "\n";
        }
    }
    return out;
}

}  // namespace todo
