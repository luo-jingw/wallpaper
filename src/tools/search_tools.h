#pragma once

#include <nlohmann/json.hpp>

#include "ai/deepseek_client.h"
#include "ai/web_search_client.h"
#include "tools/path_policy.h"
#include "tools/tool.h"

namespace todo {

// Read-only searches. Neither tool changes anything.

// Substring search in the .md and .txt files under a directory PathPolicy lets conversations read.
class SearchFilesTool : public Tool {
public:
    explicit SearchFilesTool(const PathPolicy& policy) : policy_(policy) {}
    ToolSpec spec() const override;
    ToolOutcome execute(const nlohmann::json& arguments) const override;

private:
    const PathPolicy& policy_;
};

// Web search through WebSearchClient; returns titles, links, dates, and shortened excerpts.
class WebSearchTool : public Tool {
public:
    explicit WebSearchTool(const WebSearchClient& client) : client_(client) {}
    ToolSpec spec() const override;
    ToolOutcome execute(const nlohmann::json& arguments) const override;

private:
    const WebSearchClient& client_;
};

}  // namespace todo
