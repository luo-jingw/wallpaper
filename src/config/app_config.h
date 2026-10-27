#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace todo {

class AppConfig {
public:
    static AppConfig load(const std::filesystem::path& config_file);

    const std::filesystem::path& config_file() const { return config_file_; }
    const std::filesystem::path& data_dir() const { return data_dir_; }
    const std::filesystem::path& token_file() const { return token_file_; }
    const std::string& api_host() const { return api_host_; }
    const std::string& model() const { return model_; }
    bool thinking() const { return thinking_; }
    // Extra directories the conversation tools may read; writes there need confirmation.
    const std::vector<std::filesystem::path>& allowed_dirs() const { return allowed_dirs_; }

    std::string read_api_key() const;

private:
    std::vector<std::filesystem::path> allowed_dirs_;
    std::filesystem::path config_file_;
    std::filesystem::path data_dir_;
    std::filesystem::path token_file_;
    std::string api_host_;
    std::string model_;
    bool thinking_ = false;
};

}  // namespace todo
