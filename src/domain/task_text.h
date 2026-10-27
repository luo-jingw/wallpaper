#pragma once

#include <string>

#include "domain/task.h"

namespace todo {

// The one-line task description given to the model, both in the system prompt and from list_tasks.
class TaskText {
public:
    static std::string describe(const Task& task);
};

}  // namespace todo
