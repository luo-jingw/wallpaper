#pragma once

#include <optional>
#include <string>

#include "domain/task.h"

namespace todo {

class TaskStatusCodec {
public:
    static char to_marker(TaskStatus status);
    static std::optional<TaskStatus> from_marker(char marker);
    static std::string to_name(TaskStatus status);
    static std::optional<TaskStatus> from_name(const std::string& name);
    static bool is_unfinished(TaskStatus status);
};

}  // namespace todo
