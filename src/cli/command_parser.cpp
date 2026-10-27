#include "cli/command_parser.h"

#include <charconv>
#include <optional>
#include <string>
#include <system_error>
#include <vector>

#include "util/todo_error.h"
#include "util/utf8.h"

namespace todo {

namespace {

Date parse_date_argument(const std::string& option, const std::string& value) {
    const std::optional<Date> date = Date::parse(value);
    if (!date) {
        throw TodoError(option + " expects YYYY-MM-DD, got: " + value);
    }
    return *date;
}

int parse_int_argument(const std::string& option, const std::string& value) {
    int result = 0;
    const char* const end = value.data() + value.size();
    const auto [last, error] = std::from_chars(value.data(), end, result);
    if (value.empty() || error != std::errc() || last != end) {
        throw TodoError(option + " expects an integer, got: " + value);
    }
    return result;
}

}  // namespace

Command CommandParser::parse(int argc, const wchar_t* const* argv) const {
    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) {
        args.push_back(Utf8::from_wide(argv[i]));
    }

    Command command;
    bool has_config = false;
    for (size_t i = 0; i < args.size(); ++i) {
        const std::string& arg = args[i];
        const bool has_value = i + 1 < args.size();
        if (arg == "--config" || arg == "--today" || arg == "--date" || arg == "--parent" || arg == "--anchor" ||
            arg == "--offset-x" || arg == "--offset-y" || arg == "--skill" || arg == "--chat" ||
            arg == "--color-bg" || arg == "--color-panel" || arg == "--color-text" || arg == "--color-dim" ||
            arg == "--color-accent") {
            if (!has_value) {
                throw TodoError(arg + " requires a value");
            }
            const std::string& value = args[++i];
            if (arg == "--config") {
                command.config_file = Utf8::to_path(value);
                has_config = true;
            } else if (arg == "--today") {
                command.today = parse_date_argument(arg, value);
            } else if (arg == "--date") {
                command.date = parse_date_argument(arg, value);
            } else if (arg == "--anchor") {
                command.anchor = value;
            } else if (arg == "--offset-x") {
                command.offset_x = parse_int_argument(arg, value);
            } else if (arg == "--offset-y") {
                command.offset_y = parse_int_argument(arg, value);
            } else if (arg == "--skill") {
                command.skill = value;
            } else if (arg == "--chat") {
                command.chat = value;
            } else if (arg == "--color-bg") {
                command.color_bg = value;
            } else if (arg == "--color-panel") {
                command.color_panel = value;
            } else if (arg == "--color-text") {
                command.color_text = value;
            } else if (arg == "--color-dim") {
                command.color_dim = value;
            } else if (arg == "--color-accent") {
                command.color_accent = value;
            } else {
                command.parent = value;
            }
        } else if (arg == "--with-tasks") {
            command.with_tasks = true;
        } else if (arg == "--dock-below") {
            command.dock_below = true;
        } else if (arg.rfind("--", 0) == 0) {
            throw TodoError("unknown option: " + arg);
        } else if (command.verb.empty()) {
            command.verb = arg;
        } else {
            command.positionals.push_back(arg);
        }
    }

    if (!has_config) {
        throw TodoError("--config <todo.ini> is required");
    }
    if (command.verb.empty()) {
        throw TodoError("missing command");
    }
    return command;
}

}  // namespace todo
