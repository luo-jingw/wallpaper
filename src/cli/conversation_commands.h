#pragma once

#include <string>

#include "chat/chat_session.h"
#include "cli/command.h"
#include "cli/command_context.h"

namespace todo {

// Main's conversations: ask <question>, chat new | list | show | send, skill list. Full tool access.
class ConversationCommands {
public:
    explicit ConversationCommands(const CommandContext& context) : context_(context) {}

    void ask(const Command& command) const;
    void chat(const Command& command) const;
    void skill(const Command& command) const;

private:
    void apply_skill(ChatSession& session, const Command& command) const;
    // Sends with the session's skill and the full tool set, then reports the session and its usage.
    void send_and_report(ChatSession& session, const std::string& message, const Command& command) const;
    void report_session(const ChatSession& session) const;

    const CommandContext& context_;
};

}  // namespace todo
