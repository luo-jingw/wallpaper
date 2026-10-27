#pragma once

#include <cstddef>
#include <string>

#include "ai/deepseek_client.h"
#include "ai/https_client.h"
#include "chat/chat_store.h"
#include "cli/command.h"
#include "cli/skin_output.h"
#include "config/app_config.h"
#include "domain/task_level.h"
#include "research/research_store.h"
#include "skills/skill_store.h"
#include "storage/task_repository.h"
#include "util/date.h"
#include "vcs/git_client.h"

namespace todo {

// What every command handler shares: the configuration, today's date, the skin output, the stores, and git.
// Owned by CommandRunner; handlers keep a reference.
struct CommandContext {
    const AppConfig& config;
    Date today;
    SkinOutput& output;
    TaskRepository& tasks;
    const ResearchStore& research;
    const SkillStore& skills;
    const ChatStore& chats;
    const GitClient& git;

    // Commits the content repository and reports COMMIT.
    void commit(const std::string& message) const;
    // Throws TodoError "missing argument: <name>" when the positional argument is absent.
    const std::string& require_positional(const Command& command, size_t index, const char* name) const;
    TaskLevel require_level(const Command& command, size_t index) const;
    // A DeepSeek client from the configuration; reads the API key. `http` must outlive it.
    DeepSeekClient deepseek(const HttpsClient& http) const;
};

}  // namespace todo
