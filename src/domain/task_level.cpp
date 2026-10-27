#include "domain/task_level.h"

namespace todo {

std::string TaskLevelCodec::to_name(TaskLevel level) {
    switch (level) {
        case TaskLevel::Day: return "day";
        case TaskLevel::Week: return "week";
        case TaskLevel::LongTerm: return "long";
    }
    return "day";
}

std::optional<TaskLevel> TaskLevelCodec::from_name(const std::string& name) {
    if (name == "day") return TaskLevel::Day;
    if (name == "week") return TaskLevel::Week;
    if (name == "long") return TaskLevel::LongTerm;
    return std::nullopt;
}

char TaskLevelCodec::id_prefix(TaskLevel level) {
    switch (level) {
        case TaskLevel::Day: return 'd';
        case TaskLevel::Week: return 'w';
        case TaskLevel::LongTerm: return 'l';
    }
    return 'd';
}

}  // namespace todo
