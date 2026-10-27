#include "cli/command_context.h"

#include <optional>
#include <string>

#include "util/todo_error.h"

namespace todo {

void CommandContext::commit(const std::string& message) const {
    output.commit(git.commit_all(message), message);
}

const std::string& CommandContext::require_positional(const Command& command, size_t index, const char* name) const {
    if (index >= command.positionals.size()) {
        throw TodoError(std::string("missing argument: ") + name);
    }
    return command.positionals[index];
}

TaskLevel CommandContext::require_level(const Command& command, size_t index) const {
    const std::string& name = require_positional(command, index, "level");
    const std::optional<TaskLevel> level = TaskLevelCodec::from_name(name);
    if (!level) {
        throw TodoError("level must be one of day, week, long");
    }
    return *level;
}

DeepSeekClient CommandContext::deepseek(const HttpsClient& http) const {
    return DeepSeekClient(http, config.api_host(), config.read_api_key(), config.model(), config.thinking());
}

}  // namespace todo
