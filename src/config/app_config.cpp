#include "config/app_config.h"

#include <sstream>

#include "util/text.h"
#include "util/todo_error.h"
#include "util/utf8.h"

namespace todo {

namespace {

std::string trim(const std::string& text) {
    return Text::trim(text);
}

std::filesystem::path resolve(const std::filesystem::path& base_dir, const std::string& value_utf8) {
    const std::filesystem::path value = Utf8::to_path(value_utf8);
    return std::filesystem::weakly_canonical(value.is_absolute() ? value : base_dir / value);
}

}  // namespace

AppConfig AppConfig::load(const std::filesystem::path& config_file) {
    const std::filesystem::path absolute = std::filesystem::absolute(config_file);
    const std::filesystem::path base_dir = absolute.parent_path();
    std::istringstream lines(Utf8::read_file(absolute));

    AppConfig config;
    config.config_file_ = absolute;
    bool has_data_dir = false;
    bool has_token_file = false;
    bool has_thinking = false;

    std::string line;
    while (std::getline(lines, line)) {
        const std::string stripped = trim(line);
        if (stripped.empty() || stripped[0] == '#') {
            continue;
        }
        const auto equals = stripped.find('=');
        if (equals == std::string::npos) {
            throw TodoError("invalid config line: " + stripped);
        }
        const std::string key = trim(stripped.substr(0, equals));
        const std::string value = trim(stripped.substr(equals + 1));
        if (key == "data_dir") {
            config.data_dir_ = resolve(base_dir, value);
            has_data_dir = true;
        } else if (key == "token_file") {
            config.token_file_ = resolve(base_dir, value);
            has_token_file = true;
        } else if (key == "api_host") {
            config.api_host_ = value;
        } else if (key == "model") {
            config.model_ = value;
        } else if (key == "thinking") {
            if (value != "enabled" && value != "disabled") {
                throw TodoError("thinking must be enabled or disabled");
            }
            config.thinking_ = value == "enabled";
            has_thinking = true;
        } else if (key == "allowed_dirs") {
            std::istringstream entries(value);
            std::string entry;
            while (std::getline(entries, entry, ';')) {
                const std::string dir = trim(entry);
                if (!dir.empty()) {
                    config.allowed_dirs_.push_back(resolve(base_dir, dir));
                }
            }
        } else {
            throw TodoError("unknown config key: " + key);
        }
    }

    if (!has_data_dir || !has_token_file || !has_thinking || config.api_host_.empty() || config.model_.empty()) {
        throw TodoError("config requires data_dir, token_file, api_host, model, thinking");
    }
    return config;
}

std::string AppConfig::read_api_key() const {
    if (!std::filesystem::exists(token_file_)) {
        throw TodoError("token file not found: " + Utf8::from_path(token_file_));
    }
    const std::string key = trim(Utf8::read_file(token_file_));
    if (key.empty()) {
        throw TodoError("token file is empty: " + Utf8::from_path(token_file_));
    }
    return key;
}

}  // namespace todo
