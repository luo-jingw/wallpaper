#pragma once

#include <optional>
#include <string>

#include <nlohmann/json.hpp>

namespace todo {

// Reads tool-call arguments; every accessor throws TodoError naming the offending key.
class ToolArguments {
public:
    static std::string required_string(const nlohmann::json& arguments, const std::string& key);
    static std::optional<std::string> optional_string(const nlohmann::json& arguments, const std::string& key);
    // Absent or null: nullopt. Present: must be an integer >= minimum.
    static std::optional<int> optional_int(const nlohmann::json& arguments, const std::string& key, int minimum);
};

}  // namespace todo
