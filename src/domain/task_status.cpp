#include "domain/task_status.h"

namespace todo {

char TaskStatusCodec::to_marker(TaskStatus status) {
    switch (status) {
        case TaskStatus::Todo: return ' ';
        case TaskStatus::Doing: return '~';
        case TaskStatus::Done: return 'x';
        case TaskStatus::Blocked: return '!';
        case TaskStatus::Dropped: return '-';
        case TaskStatus::Rolled: return '>';
    }
    return ' ';
}

std::optional<TaskStatus> TaskStatusCodec::from_marker(char marker) {
    switch (marker) {
        case ' ': return TaskStatus::Todo;
        case '~': return TaskStatus::Doing;
        case 'x':
        case 'X': return TaskStatus::Done;
        case '!': return TaskStatus::Blocked;
        case '-': return TaskStatus::Dropped;
        case '>': return TaskStatus::Rolled;
        default: return std::nullopt;
    }
}

std::string TaskStatusCodec::to_name(TaskStatus status) {
    switch (status) {
        case TaskStatus::Todo: return "todo";
        case TaskStatus::Doing: return "doing";
        case TaskStatus::Done: return "done";
        case TaskStatus::Blocked: return "blocked";
        case TaskStatus::Dropped: return "dropped";
        case TaskStatus::Rolled: return "rolled";
    }
    return "todo";
}

std::optional<TaskStatus> TaskStatusCodec::from_name(const std::string& name) {
    if (name == "todo") return TaskStatus::Todo;
    if (name == "doing") return TaskStatus::Doing;
    if (name == "done") return TaskStatus::Done;
    if (name == "blocked") return TaskStatus::Blocked;
    if (name == "dropped") return TaskStatus::Dropped;
    if (name == "rolled") return TaskStatus::Rolled;
    return std::nullopt;
}

bool TaskStatusCodec::is_unfinished(TaskStatus status) {
    return status == TaskStatus::Todo || status == TaskStatus::Doing || status == TaskStatus::Blocked;
}

}  // namespace todo
