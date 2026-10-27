#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace todo {

enum class AccessKind { Read, Write };

struct AccessDecision {
    std::filesystem::path resolved;
    bool allowed = false;
    bool needs_confirmation = false;
    bool in_content_repo = false;
    std::string reason;
};

// The only place that decides whether a conversation tool may touch a path.
class PathPolicy {
public:
    PathPolicy(std::filesystem::path data_dir, std::vector<std::filesystem::path> allowed_dirs);

    // Relative paths resolve against data_dir. Symlinks and ".." are resolved before the check;
    // comparison is case-insensitive, as on NTFS.
    AccessDecision check(const std::string& requested, AccessKind kind) const;

    const std::filesystem::path& data_dir() const { return data_dir_; }

private:
    std::filesystem::path data_dir_;
    std::vector<std::filesystem::path> allowed_dirs_;
};

}  // namespace todo
