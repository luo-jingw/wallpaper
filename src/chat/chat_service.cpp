#include "chat/chat_service.h"

#include <vector>

#include "domain/task.h"
#include "domain/task_level.h"
#include "domain/task_text.h"
#include "storage/task_file.h"
#include "tools/tool_status.h"

namespace todo {

namespace {

constexpr int kMaxToolRounds = 8;

const std::string kBasePrompt =
    "You are a concise assistant embedded in a personal task manager. "
    "Answer in the language of the question. Prefer short, direct answers.";

ChatMessage make_message(const std::string& role, const std::string& content) {
    return ChatMessage{role, content, {}, {}};
}

}  // namespace

ChatService::ChatService(const ChatStore& store, const DeepSeekClient& client, const TaskRepository& tasks,
                         const SkillStore& skills, const ToolRegistry& tools)
    : store_(store), client_(client), tasks_(tasks), skills_(skills), tools_(tools) {}

ChatExchange ChatService::send(ChatSession& session, const std::string& message, bool with_tasks,
                               const Date& today) const {
    Skill skill;
    if (!session.skill.empty()) {
        skill = skills_.load(session.skill);
    }
    const ChatProfile profile{skill.system_prompt.empty() ? kBasePrompt : skill.system_prompt, skill.tools, 0,
                              with_tasks};
    return send(session, message, profile, today);
}

ChatExchange ChatService::send(ChatSession& session, const std::string& message, const ChatProfile& profile,
                               const Date& today) const {
    const std::string system = system_prompt(profile.system_prompt, profile.with_tasks, today);
    std::vector<ChatMessage> messages = {make_message("system", system)};
    const size_t first_turn = profile.history_turns == 0 || session.turns.size() <= profile.history_turns
                                  ? 0
                                  : session.turns.size() - profile.history_turns;
    for (size_t i = first_turn; i < session.turns.size(); ++i) {
        messages.push_back(make_message(session.turns[i].role, session.turns[i].content));
    }
    messages.push_back(make_message("user", message));
    const size_t first_messages = messages.size();

    std::vector<ToolRecord> records;
    ChatCompletion completion;
    int rounds = 0;
    int tokens_in = 0;
    int tokens_out = 0;
    long long latency_ms = 0;
    const bool can_use_tools = !profile.tools.empty();
    while (true) {
        ++rounds;
        const bool send_tools = can_use_tools && rounds < kMaxToolRounds;
        CompletionOptions options;
        if (send_tools) {
            options.tools = tools_.specs(profile.tools);
        }
        completion = client_.complete(messages, options);
        tokens_in += completion.tokens_in;
        tokens_out += completion.tokens_out;
        latency_ms += completion.latency_ms;
        if (completion.tool_calls.empty() || !send_tools) {
            break;
        }
        messages.push_back(ChatMessage{"assistant", completion.content, completion.tool_calls, {}});
        for (const ToolCall& call : completion.tool_calls) {
            const ToolOutcome outcome = tools_.execute(call, profile.tools);
            records.push_back(ToolRecord{call.name, call.arguments_json, outcome.summary,
                                         ToolStatusCodec::to_name(outcome.status)});
            messages.push_back(ChatMessage{"tool", outcome.result_text, {}, call.id});
        }
    }

    ChatTurn user_turn;
    user_turn.role = "user";
    user_turn.content = message;
    ChatTurn reply;
    reply.role = "assistant";
    reply.content = completion.content;
    reply.tokens_in = tokens_in;
    reply.tokens_out = tokens_out;
    reply.latency_ms = latency_ms;
    reply.tools = std::move(records);
    session.turns.push_back(user_turn);
    session.turns.push_back(reply);
    store_.save(session);
    return ChatExchange{reply, first_messages, system.size(), rounds, can_use_tools};
}

std::string ChatService::system_prompt(const std::string& skill_prompt, bool with_tasks, const Date& today) const {
    const std::string now = DateTime::now_local_minutes();
    std::string prompt = skill_prompt + "\nNow is " + today.to_string() + " " + now.substr(11) + " (" +
                         today.weekday_name() + ", " + today.iso_week().to_string() + "; " +
                         DateTime::local_zone_label() + "). Times the user gives are in this time zone.";
    if (!with_tasks) {
        return prompt;
    }
    prompt += "\nThe user's current tasks follow.\n";
    for (const TaskLevel level : {TaskLevel::Day, TaskLevel::Week, TaskLevel::LongTerm}) {
        const TaskFile file = tasks_.load(level, today);
        prompt += "\n## " + TaskLevelCodec::to_name(level) + " (" + file.heading + ")\n";
        for (const Task& task : file.tasks) {
            prompt += TaskText::describe(task);
        }
    }
    return prompt;
}

}  // namespace todo
