#pragma once

#include <optional>
#include <string>

#include "domain/task.h"

namespace todo {

class TaskLevelCodec {
public:
    static std::string to_name(TaskLevel level);  // day | week | long
    static std::optional<TaskLevel> from_name(const std::string& name);
    static char id_prefix(TaskLevel level);  // d | w | l
};

}  // namespace todo
