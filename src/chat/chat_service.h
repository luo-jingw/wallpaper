#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "ai/deepseek_client.h"
#include "chat/chat_session.h"
#include "chat/chat_store.h"
#include "skills/skill_store.h"
#include "storage/task_repository.h"
#include "tools/tool_registry.h"
#include "util/date.h"

namespace todo {

// What one conversation sends besides its turns.
struct ChatProfile {
    std::string system_prompt;
    std::vector<std::string> tools;  // empty: the request has no tools field
    size_t history_turns = 0;        // the latest turns sent as context; 0: all
    bool with_tasks = false;         // the current tasks are appended to the system prompt
};

struct ChatExchange {
    ChatTurn reply;
    size_t request_messages;  // including the system message, first completion only
    size_t system_chars;
    int completion_rounds = 1;
    bool sent_tools = false;
};

class ChatService {
public:
    ChatService(const ChatStore& store, const DeepSeekClient& client, const TaskRepository& tasks,
                const SkillStore& skills, const ToolRegistry& tools);

    // The session's skill (or the base prompt without tools), all turns as context.
    ChatExchange send(ChatSession& session, const std::string& message, bool with_tasks, const Date& today) const;
    ChatExchange send(ChatSession& session, const std::string& message, const ChatProfile& profile,
                      const Date& today) const;

private:
    std::string system_prompt(const std::string& skill_prompt, bool with_tasks, const Date& today) const;

    const ChatStore& store_;
    const DeepSeekClient& client_;
    const TaskRepository& tasks_;
    const SkillStore& skills_;
    const ToolRegistry& tools_;
};

}  // namespace todo
