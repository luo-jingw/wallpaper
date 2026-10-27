#include "domain/task_text.h"

#include "domain/task_place.h"
#include "domain/task_status.h"

namespace todo {

std::string TaskText::describe(const Task& task) {
    std::string line = "- [" + TaskStatusCodec::to_name(task.status) + "] " + task.title + " (id " + task.id +
                       ", created " + task.created;
    if (task.rolled > 0) {
        line += ", rolled over " + std::to_string(task.rolled) + " times";
    }
    if (task.parent) {
        line += ", parent " + *task.parent;
    }
    if (task.place != TaskPlace::Any) {
        line += ", place " + TaskPlaceCodec::to_name(task.place);
    }
    if (task.estimate_minutes) {
        line += ", estimate " + std::to_string(*task.estimate_minutes) + " min";
    }
    line += ")";
    if (!task.notes.empty()) {
        line += " latest note: " + task.notes.back();
    }
    return line + "\n";
}

}  // namespace todo
