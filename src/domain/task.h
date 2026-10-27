#pragma once

#include <optional>
#include <string>
#include <vector>

namespace todo {

enum class TaskLevel { Day, Week, LongTerm };

enum class TaskStatus { Todo, Doing, Done, Blocked, Dropped, Rolled };

enum class TaskPlace { Any, Lab, Class, Remote };

struct Task {
    std::string id;  // empty for a task line written by hand without metadata
    std::string title;
    TaskStatus status = TaskStatus::Todo;
    std::string created;  // YYYY-MM-DD
    int rolled = 0;
    std::optional<std::string> parent;
    TaskPlace place = TaskPlace::Any;
    std::optional<int> estimate_minutes;
    std::vector<std::string> notes;
};

}  // namespace todo
