#include "tools/tool_registry.h"

#include <algorithm>
#include <exception>
#include <utility>

#include "tools/tool_status.h"
#include "util/todo_error.h"

namespace todo {

void ToolRegistry::add(std::unique_ptr<Tool> tool) {
    tools_.push_back(std::move(tool));
}

std::vector<ToolSpec> ToolRegistry::specs(const std::vector<std::string>& names) const {
    std::vector<ToolSpec> result;
    for (const std::string& name : names) {
        const Tool* tool = find(name);
        if (!tool) {
            throw TodoError("unknown tool: " + name);
        }
        result.push_back(tool->spec());
    }
    return result;
}

ToolOutcome ToolRegistry::execute(const ToolCall& call, const std::vector<std::string>& allowed_names) const {
    const Tool* tool = find(call.name);
    if (!tool || std::find(allowed_names.begin(), allowed_names.end(), call.name) == allowed_names.end()) {
        return ToolOutcome{ToolStatus::Denied, "error: tool not enabled: " + call.name, false, "not enabled"};
    }
    const nlohmann::json arguments = nlohmann::json::parse(call.arguments_json, nullptr, false);
    if (arguments.is_discarded() || !arguments.is_object()) {
        return ToolOutcome{ToolStatus::Error, "error: arguments are not a JSON object", false, "invalid arguments"};
    }
    try {
        return tool->execute(arguments);
    } catch (const std::exception& error) {
        return ToolOutcome{ToolStatus::Error, std::string("error: ") + error.what(), false, error.what()};
    }
}

std::vector<std::string> ToolRegistry::names() const {
    std::vector<std::string> result;
    for (const std::unique_ptr<Tool>& tool : tools_) {
        result.push_back(tool->spec().name);
    }
    return result;
}

const Tool* ToolRegistry::find(const std::string& name) const {
    for (const std::unique_ptr<Tool>& tool : tools_) {
        if (tool->spec().name == name) {
            return tool.get();
        }
    }
    return nullptr;
}

}  // namespace todo
