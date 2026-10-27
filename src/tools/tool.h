#pragma once

#include <string>

#include <nlohmann/json.hpp>

#include "ai/deepseek_client.h"
#include "tools/tool_status.h"

namespace todo {

struct ToolOutcome {
    ToolStatus status = ToolStatus::Ok;
    std::string result_text;           // sent back to the model as the tool message
    bool changed_content_repo = false;  // the command commits the content repository afterwards
    std::string summary;               // one line for the TOOL record
};

class Tool {
public:
    virtual ~Tool() = default;
    virtual ToolSpec spec() const = 0;
    // May throw TodoError for invalid arguments; ToolRegistry turns any exception into an Error outcome.
    virtual ToolOutcome execute(const nlohmann::json& arguments) const = 0;
};

}  // namespace todo
