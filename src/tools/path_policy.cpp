#include "tools/path_policy.h"

#include <windows.h>

#include <system_error>
#include <utility>

#include "util/utf8.h"

namespace todo {

namespace {

bool same_component(const std::filesystem::path& a, const std::filesystem::path& b) {
    const std::wstring& left = a.native();
    const std::wstring& right = b.native();
    return CompareStringOrdinal(left.c_str(), static_cast<int>(left.size()), right.c_str(),
                                static_cast<int>(right.size()), TRUE) == CSTR_EQUAL;
}

bool is_within(const std::filesystem::path& root, const std::filesystem::path& path) {
    auto root_part = root.begin();
    auto path_part = path.begin();
    for (; root_part != root.end(); ++root_part, ++path_part) {
        if (root_part->empty()) {
            continue;  // trailing separator
        }
        if (path_part == path.end() || !same_component(*root_part, *path_part)) {
            return false;
        }
    }
    return true;
}

std::filesystem::path canonical_or_empty(const std::filesystem::path& path) {
    std::error_code error;
    std::filesystem::path result = std::filesystem::weakly_canonical(path, error);
    return error ? std::filesystem::path() : result;
}

}  // namespace

PathPolicy::PathPolicy(std::filesystem::path data_dir, std::vector<std::filesystem::path> allowed_dirs)
    : data_dir_(canonical_or_empty(data_dir)) {
    for (const std::filesystem::path& dir : allowed_dirs) {
        allowed_dirs_.push_back(canonical_or_empty(dir));
    }
}

AccessDecision PathPolicy::check(const std::string& requested, AccessKind kind) const {
    AccessDecision decision;
    if (requested.empty()) {
        decision.reason = "empty path";
        return decision;
    }
    const std::filesystem::path path = Utf8::to_path(requested);
    decision.resolved = canonical_or_empty(path.is_absolute() ? path : data_dir_ / path);
    if (decision.resolved.empty()) {
        decision.reason = "path cannot be resolved";
        return decision;
    }
    if (is_within(data_dir_, decision.resolved)) {
        if (is_within(data_dir_ / ".git", decision.resolved)) {
            decision.reason = ".git is not accessible";
            return decision;
        }
        decision.allowed = true;
        decision.in_content_repo = true;
        decision.reason = "content repository";
        return decision;
    }
    for (const std::filesystem::path& dir : allowed_dirs_) {
        if (!dir.empty() && is_within(dir, decision.resolved)) {
            decision.allowed = true;
            decision.needs_confirmation = kind == AccessKind::Write;
            decision.reason = "allowed directory " + Utf8::from_path(dir);
            return decision;
        }
    }
    decision.reason = "outside allowed directories";
    return decision;
}

}  // namespace todo
