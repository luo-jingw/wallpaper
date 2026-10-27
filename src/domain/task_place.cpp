#include "domain/task_place.h"

namespace todo {

std::string TaskPlaceCodec::to_name(TaskPlace place) {
    switch (place) {
        case TaskPlace::Any: return "any";
        case TaskPlace::Lab: return "lab";
        case TaskPlace::Class: return "class";
        case TaskPlace::Remote: return "remote";
    }
    return "any";
}

std::optional<TaskPlace> TaskPlaceCodec::from_name(const std::string& name) {
    if (name == "any") return TaskPlace::Any;
    if (name == "lab") return TaskPlace::Lab;
    if (name == "class") return TaskPlace::Class;
    if (name == "remote") return TaskPlace::Remote;
    return std::nullopt;
}

}  // namespace todo
