#pragma once

#include "chat/chat_session.h"
#include "cli/command.h"
#include "cli/command_context.h"

namespace todo {

// The mascot's read-only conversation: companion show | send <message> | new (<data_dir>/companion/).
class CompanionCommands {
public:
    explicit CompanionCommands(const CommandContext& context) : context_(context) {}

    void run(const Command& command) const;

private:
    // CHAT, then the latest turns with their TOOL records.
    void report(const ChatSession& session) const;

    const CommandContext& context_;
};

}  // namespace todo
