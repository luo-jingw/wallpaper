#pragma once

#include <memory>
#include <string>
#include <vector>

#include "ai/deepseek_client.h"
#include "tools/tool.h"

namespace todo {

class ToolRegistry {
public:
    void add(std::unique_ptr<Tool> tool);
    // Throws TodoError for a name that is not registered.
    std::vector<ToolSpec> specs(const std::vector<std::string>& names) const;
    // A call to a tool outside allowed_names is denied without running it.
    ToolOutcome execute(const ToolCall& call, const std::vector<std::string>& allowed_names) const;
    std::vector<std::string> names() const;

private:
    const Tool* find(const std::string& name) const;

    std::vector<std::unique_ptr<Tool>> tools_;
};

}  // namespace todo
