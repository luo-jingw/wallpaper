#include "cli/conversation_tools.h"

#include <memory>
#include <string>
#include <vector>

#include "companion/companion_service.h"
#include "tools/file_tools.h"
#include "tools/plan_tools.h"
#include "tools/search_tools.h"
#include "tools/task_tools.h"
#include "util/todo_error.h"

namespace todo {

ConversationTools::ConversationTools(const CommandContext& context, ToolAccess access,
                                     const std::wstring& anchor_title, const HttpsClient& http)
    : policy_(context.config.data_dir(), context.config.allowed_dirs()),
      plans_(context.config.data_dir()),
      web_(http, context.config.api_host(), context.config.read_api_key()) {
    const FileToolContext files{policy_, confirm_, anchor_title};
    const TaskToolContext task_tools{context.tasks, context.today};
    if (access == ToolAccess::ReadOnly) {
        registry_.add(std::make_unique<ListTasksTool>(task_tools));
        registry_.add(std::make_unique<ListDirTool>(files));
        registry_.add(std::make_unique<ReadFileTool>(files));
        registry_.add(std::make_unique<SearchFilesTool>(policy_));
        registry_.add(std::make_unique<WebSearchTool>(web_));
        // The registry and the companion's allow-list are two barriers; they must name the same tools.
        if (registry_.names() != CompanionService::read_only_tools()) {
            throw TodoError("read-only tool registry does not match CompanionService::read_only_tools()");
        }
        return;
    }
    registry_.add(std::make_unique<ListTasksTool>(task_tools));
    registry_.add(std::make_unique<AddTaskTool>(task_tools));
    registry_.add(std::make_unique<SetTaskStatusTool>(task_tools));
    registry_.add(std::make_unique<AddTaskNoteTool>(task_tools));
    registry_.add(std::make_unique<SetTaskPlaceTool>(task_tools));
    registry_.add(std::make_unique<ListDirTool>(files));
    registry_.add(std::make_unique<ReadFileTool>(files));
    registry_.add(std::make_unique<WriteFileTool>(files));
    registry_.add(std::make_unique<ReplaceInFileTool>(files));
    registry_.add(std::make_unique<ProposePlanTool>(PlanToolContext{plans_, context.today,
                                                                    DateTime::now_local_minutes()}));
}

}  // namespace todo
