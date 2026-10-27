#include "cli/command_runner.h"

#include <filesystem>
#include <string>

#include "util/todo_error.h"
#include "util/utf8.h"

namespace todo {

CommandRunner::CommandRunner(const AppConfig& config, Date today, SkinOutput& output, TaskRepository& tasks,
                             const ResearchStore& research, const ChatStore& chats, const GitClient& git)
    : skills_(config.data_dir()),
      context_{config, today, output, tasks, research, skills_, chats, git},
      task_commands_(context_),
      research_commands_(context_),
      conversation_commands_(context_),
      companion_commands_(context_),
      plan_commands_(context_),
      prompt_command_(context_) {}

void CommandRunner::run(const Command& command) {
    const std::string& verb = command.verb;
    if (verb == "config") {
        run_config();
    } else if (verb == "rollover") {
        context_.output.ok("rollover ensured for " + context_.today.to_string());
    } else if (verb == "list") {
        task_commands_.list(command);
    } else if (verb == "add") {
        task_commands_.add(command);
    } else if (verb == "status") {
        task_commands_.status(command);
    } else if (verb == "note") {
        task_commands_.note(command);
    } else if (verb == "place") {
        task_commands_.place(command);
    } else if (verb == "estimate") {
        task_commands_.estimate(command);
    } else if (verb == "sync") {
        task_commands_.sync();
    } else if (verb == "research") {
        research_commands_.run(command);
    } else if (verb == "ask") {
        conversation_commands_.ask(command);
    } else if (verb == "chat") {
        conversation_commands_.chat(command);
    } else if (verb == "skill") {
        conversation_commands_.skill(command);
    } else if (verb == "companion") {
        companion_commands_.run(command);
    } else if (verb == "prompt") {
        prompt_command_.run(command);
    } else if (verb == "plan") {
        plan_commands_.run(command);
    } else {
        throw TodoError("unknown command: " + verb);
    }
}

void CommandRunner::run_config() const {
    const AppConfig& config = context_.config;
    SkinOutput& output = context_.output;
    output.config("config_file", Utf8::from_path(config.config_file()));
    output.config("data_dir", Utf8::from_path(config.data_dir()));
    output.config("token_file", Utf8::from_path(config.token_file()));
    output.config("token_present", std::filesystem::exists(config.token_file()) ? "yes" : "no");
    output.config("api_host", config.api_host());
    output.config("model", config.model());
    output.config("thinking", config.thinking() ? "enabled" : "disabled");
    for (const std::filesystem::path& dir : config.allowed_dirs()) {
        output.config("allowed_dir", Utf8::from_path(dir));
    }
    output.config("today", context_.today.to_string());
    output.config("iso_week", context_.today.iso_week().to_string());
}

}  // namespace todo
