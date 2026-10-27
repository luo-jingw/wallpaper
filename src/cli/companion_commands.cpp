#include "cli/companion_commands.h"

#include <optional>
#include <string>

#include "ai/deepseek_client.h"
#include "ai/https_client.h"
#include "chat/chat_service.h"
#include "chat/chat_store.h"
#include "cli/conversation_tools.h"
#include "companion/companion_service.h"
#include "util/text.h"
#include "util/todo_error.h"

namespace todo {

namespace {

constexpr size_t kTitleCodePoints = 30;
// The mascot's bubble shows the latest turns only.
constexpr size_t kShownTurns = 8;

}  // namespace

void CompanionCommands::run(const Command& command) const {
    const std::string& action = context_.require_positional(command, 0, "companion action");
    const ChatStore store(context_.config.data_dir(), "companion");
    const CompanionService companion(store);
    if (action == "show") {
        if (const std::optional<ChatSession> session = companion.current(context_.today)) {
            report(*session);
        }
        context_.output.ok("companion");
    } else if (action == "new") {
        const ChatSession session = companion.start(context_.today);
        report(session);
        context_.commit("companion " + session.id + ": new");
        context_.output.ok("started " + session.id);
    } else if (action == "send") {
        const std::string& message = context_.require_positional(command, 1, "message");
        const HttpsClient http;
        // Only the read-only tools exist in this process.
        const ConversationTools tools(context_, ToolAccess::ReadOnly, std::wstring(), http);
        const DeepSeekClient client = context_.deepseek(http);
        const ChatService chat(store, client, context_.tasks, context_.skills, tools.registry());
        ChatSession session;
        const ChatExchange exchange = companion.send(chat, message, context_.today, session);
        report(session);
        context_.output.usage(exchange);
        context_.commit("companion " + session.id + ": " +
                        Text::truncate_code_points(Text::single_line(message), kTitleCodePoints));
        context_.output.ok("sent " + session.id);
    } else {
        throw TodoError("companion action must be one of show, send, new");
    }
}

void CompanionCommands::report(const ChatSession& session) const {
    context_.output.chat(ChatSummary{session.id, session.title, session.created,
                                     static_cast<int>(session.turns.size()), session.skill});
    const size_t first = session.turns.size() > kShownTurns ? session.turns.size() - kShownTurns : 0;
    for (size_t i = first; i < session.turns.size(); ++i) {
        for (const ToolRecord& record : session.turns[i].tools) {
            context_.output.tool(record);
        }
        context_.output.turn(session.turns[i]);
    }
}

}  // namespace todo
