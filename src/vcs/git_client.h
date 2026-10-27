#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "process/process_runner.h"

namespace todo {

// Sole writer of the content repository's git history.
class GitClient {
public:
    GitClient(const ProcessRunner& runner, std::filesystem::path repo_dir);

    // Creates the repository (with LF line endings enforced) if it does not exist yet.
    void ensure_repository() const;
    // Stages everything and commits. Returns the short sha, or nullopt when there was nothing to commit.
    std::optional<std::string> commit_all(const std::string& message_utf8) const;

private:
    ProcessResult git(const std::vector<std::wstring>& args) const;
    ProcessResult git_checked(const std::vector<std::wstring>& args) const;

    const ProcessRunner& runner_;
    std::filesystem::path repo_dir_;
    std::filesystem::path git_exe_;
};

}  // namespace todo
