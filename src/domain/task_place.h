#pragma once

#include <optional>
#include <string>

#include "domain/task.h"

namespace todo {

class TaskPlaceCodec {
public:
    static std::string to_name(TaskPlace place);
    static std::optional<TaskPlace> from_name(const std::string& name);
};

}  // namespace todo
