#include "cli/conversation_commands.h"

#include <string>
#include <vector>

#include "ai/deepseek_client.h"
#include "ai/https_client.h"
#include "chat/chat_service.h"
#include "cli/conversation_tools.h"
#include "skills/skill.h"
#include "util/text.h"
#include "util/todo_error.h"
#include "util/utf8.h"

namespace todo {

namespace {

constexpr size_t kChatTitleCodePoints = 30;

}  // namespace

void ConversationCommands::ask(const Command& command) const {
    const std::string question = Text::trim(context_.require_positional(command, 0, "question"));
    if (question.empty()) {
        throw TodoError("question is empty");
    }
    ChatSession session =
        context_.chats.create(Text::truncate_code_points(Text::single_line(question), kChatTitleCodePoints),
                              context_.today, DateTime::now_local_minutes());
    apply_skill(session, command);
    send_and_report(session, question, command);
    context_.commit("ask " + session.id + ": " + session.title);
    context_.output.ok("asked " + session.id);
}

void ConversationCommands::chat(const Command& command) const {
    const std::string& action = context_.require_positional(command, 0, "chat action");
    if (action == "list") {
        const std::vector<ChatSummary> summaries = context_.chats.list();
        for (const ChatSummary& summary : summaries) {
            context_.output.chat(summary);
        }
        context_.output.ok(std::to_string(summaries.size()) + " chats");
    } else if (action == "new") {
        const std::string title = Text::single_line(context_.require_positional(command, 1, "title"));
        if (title.empty()) {
            throw TodoError("title is empty");
        }
        ChatSession session = context_.chats.create(title, context_.today, DateTime::now_local_minutes());
        apply_skill(session, command);
        context_.chats.save(session);
        report_session(session);
        context_.commit("chat new " + session.id + ": " + session.title);
        context_.output.ok("created " + session.id);
    } else if (action == "show") {
        report_session(context_.chats.load(context_.require_positional(command, 1, "chat_id")));
        context_.output.ok("shown");
    } else if (action == "send") {
        ChatSession session = context_.chats.load(context_.require_positional(command, 1, "chat_id"));
        const std::string message = Text::trim(context_.require_positional(command, 2, "message"));
        if (message.empty()) {
            throw TodoError("message is empty");
        }
        send_and_report(session, message, command);
        context_.commit("chat " + session.id + ": " +
                        Text::truncate_code_points(Text::single_line(message), kChatTitleCodePoints));
        context_.output.ok("sent " + session.id);
    } else {
        throw TodoError("chat action must be one of new, list, show, send");
    }
}

void ConversationCommands::skill(const Command& command) const {
    const std::string& action = context_.require_positional(command, 0, "skill action");
    if (action != "list") {
        throw TodoError("skill action must be list");
    }
    context_.skills.ensure_defaults();
    const std::vector<Skill> skills = context_.skills.list();
    for (const Skill& skill : skills) {
        context_.output.skill(skill);
    }
    context_.output.ok(std::to_string(skills.size()) + " skills");
    context_.commit("skill defaults");
}

void ConversationCommands::apply_skill(ChatSession& session, const Command& command) const {
    if (!command.skill) {
        return;
    }
    context_.skills.ensure_defaults();
    context_.skills.load(*command.skill);
    session.skill = *command.skill;
}

void ConversationCommands::send_and_report(ChatSession& session, const std::string& message,
                                           const Command& command) const {
    context_.skills.ensure_defaults();
    const HttpsClient http;
    const ConversationTools tools(context_, ToolAccess::Full,
                                  command.anchor ? Utf8::to_wide(*command.anchor) : std::wstring(), http);
    const DeepSeekClient client = context_.deepseek(http);
    const ChatService service(context_.chats, client, context_.tasks, context_.skills, tools.registry());
    const ChatExchange exchange = service.send(session, message, command.with_tasks, context_.today);
    report_session(session);
    context_.output.usage(exchange);
    context_.output.config("tools", exchange.sent_tools ? "yes" : "no");
    context_.output.config("rounds", std::to_string(exchange.completion_rounds));
}

void ConversationCommands::report_session(const ChatSession& session) const {
    context_.output.chat(ChatSummary{session.id, session.title, session.created,
                                     static_cast<int>(session.turns.size()), session.skill});
    for (const ChatTurn& turn : session.turns) {
        for (const ToolRecord& record : turn.tools) {
            context_.output.tool(record);
        }
        context_.output.turn(turn);
    }
}

}  // namespace todo
