#pragma once

#include <string>

#include "ai/https_client.h"
#include "ai/web_search_client.h"
#include "cli/command_context.h"
#include "planning/plan_store.h"
#include "prompt/confirm_window.h"
#include "tools/path_policy.h"
#include "tools/tool_registry.h"

namespace todo {

enum class ToolAccess {
    Full,      // Main's conversations: task, file, and plan tools; writes may ask for confirmation
    ReadOnly,  // the mascot's conversations: CompanionService::read_only_tools() only
};

// The only place that builds a conversation's ToolRegistry. Owns the objects the tools refer to, so it must
// outlive every use of registry().
class ConversationTools {
public:
    // anchor_title: the skin window write confirmations are placed on; may be empty.
    ConversationTools(const CommandContext& context, ToolAccess access, const std::wstring& anchor_title,
                      const HttpsClient& http);
    ConversationTools(const ConversationTools&) = delete;
    ConversationTools& operator=(const ConversationTools&) = delete;

    const ToolRegistry& registry() const { return registry_; }

private:
    PathPolicy policy_;
    ConfirmWindow confirm_;
    PlanStore plans_;
    WebSearchClient web_;
    ToolRegistry registry_;
};

}  // namespace todo
