#pragma once

#include "chat/chat_store.h"
#include "cli/command.h"
#include "cli/command_context.h"
#include "cli/companion_commands.h"
#include "cli/conversation_commands.h"
#include "cli/plan_commands.h"
#include "cli/prompt_command.h"
#include "cli/research_commands.h"
#include "cli/skin_output.h"
#include "cli/task_commands.h"
#include "config/app_config.h"
#include "research/research_store.h"
#include "skills/skill_store.h"
#include "storage/task_repository.h"
#include "util/date.h"
#include "vcs/git_client.h"

namespace todo {

// Dispatches a parsed command to the handler for its area; answers "config" and "rollover" itself.
class CommandRunner {
public:
    CommandRunner(const AppConfig& config, Date today, SkinOutput& output, TaskRepository& tasks,
                  const ResearchStore& research, const ChatStore& chats, const GitClient& git);
    CommandRunner(const CommandRunner&) = delete;
    CommandRunner& operator=(const CommandRunner&) = delete;

    void run(const Command& command);

private:
    void run_config() const;

    SkillStore skills_;
    CommandContext context_;  // refers to skills_, declared before it
    TaskCommands task_commands_;
    ResearchCommands research_commands_;
    ConversationCommands conversation_commands_;
    CompanionCommands companion_commands_;
    PlanCommands plan_commands_;
    PromptCommand prompt_command_;
};

}  // namespace todo
