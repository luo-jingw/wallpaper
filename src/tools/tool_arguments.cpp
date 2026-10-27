#include "tools/tool_arguments.h"

#include "util/todo_error.h"

namespace todo {

std::string ToolArguments::required_string(const nlohmann::json& arguments, const std::string& key) {
    const std::optional<std::string> value = optional_string(arguments, key);
    if (!value) {
        throw TodoError("missing argument: " + key);
    }
    return *value;
}

std::optional<std::string> ToolArguments::optional_string(const nlohmann::json& arguments, const std::string& key) {
    if (!arguments.contains(key) || arguments[key].is_null()) {
        return std::nullopt;
    }
    if (!arguments[key].is_string()) {
        throw TodoError("argument must be a string: " + key);
    }
    return arguments[key].get<std::string>();
}

std::optional<int> ToolArguments::optional_int(const nlohmann::json& arguments, const std::string& key, int minimum) {
    if (!arguments.contains(key) || arguments[key].is_null()) {
        return std::nullopt;
    }
    if (!arguments[key].is_number_integer()) {
        throw TodoError("argument must be an integer: " + key);
    }
    const long long value = arguments[key].get<long long>();
    if (value < minimum || value > 100000) {
        throw TodoError("argument out of range: " + key);
    }
    return static_cast<int>(value);
}

}  // namespace todo
