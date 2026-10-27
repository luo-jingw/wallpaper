#include "tools/file_tools.h"

#include <algorithm>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

#include "tools/tool_arguments.h"
#include "util/todo_error.h"
#include "util/utf8.h"

namespace todo {

namespace {

constexpr std::uintmax_t kMaxFileBytes = 65536;
constexpr size_t kMaxDirEntries = 200;

const std::string kPathProperty =
    R"("path":{"type":"string","description":"Path relative to the content repository, or an absolute path inside an allowed directory"})";

ToolOutcome denied(const std::string& reason) {
    return ToolOutcome{ToolStatus::Denied, "denied: " + reason, false, reason};
}

ToolOutcome error(const std::string& reason) {
    return ToolOutcome{ToolStatus::Error, "error: " + reason, false, reason};
}

AccessDecision check_path(const FileToolContext& context, const std::string& requested, AccessKind kind) {
    AccessDecision decision = context.policy.check(requested, kind);
    if (!decision.allowed) {
        return decision;
    }
    if (decision.needs_confirmation) {
        const ConfirmRequest request{
            L"Allow write?",
            Utf8::to_wide(Utf8::from_path(decision.resolved)),
            context.anchor_title,
        };
        if (!context.confirm.run(request)) {
            decision.allowed = false;
            decision.reason = "denied by user";
        }
    }
    return decision;
}

bool has_nul(const std::string& text) {
    return text.find('\0') != std::string::npos;
}

ToolOutcome read_limited(const std::filesystem::path& path) {
    std::error_code size_error;
    const std::uintmax_t size = std::filesystem::file_size(path, size_error);
    if (size_error) {
        return error("cannot read file: " + Utf8::from_path(path));
    }
    if (size > kMaxFileBytes) {
        return error("file larger than " + std::to_string(kMaxFileBytes) + " bytes");
    }
    const std::string content = Utf8::read_file(path);
    if (has_nul(content) || !Utf8::is_valid(content)) {
        return error("file is not valid UTF-8 text");
    }
    return ToolOutcome{ToolStatus::Ok, content, false, Utf8::from_path(path) + " " + std::to_string(content.size()) + " bytes"};
}

}  // namespace

ToolSpec ListDirTool::spec() const {
    return ToolSpec{
        "list_dir",
        "List files and directories. Default path is the content repository root. Not recursive.",
        R"({"type":"object","properties":{)" + kPathProperty + R"(}})",
    };
}

ToolOutcome ListDirTool::execute(const nlohmann::json& arguments) const {
    const std::string requested = ToolArguments::optional_string(arguments, "path").value_or(".");
    const AccessDecision decision = check_path(context_, requested, AccessKind::Read);
    if (!decision.allowed) {
        return denied(decision.reason);
    }
    std::error_code error_code;
    if (!std::filesystem::is_directory(decision.resolved, error_code)) {
        return error("not a directory: " + Utf8::from_path(decision.resolved));
    }
    std::vector<std::string> names;
    for (const std::filesystem::directory_entry& entry :
         std::filesystem::directory_iterator(decision.resolved, error_code)) {
        if (error_code) {
            break;
        }
        const std::string name = Utf8::from_path(entry.path().filename());
        if (name == ".git") {
            continue;
        }
        names.push_back(name + (entry.is_directory() ? "/" : ""));
        if (names.size() >= kMaxDirEntries) {
            break;
        }
    }
    std::sort(names.begin(), names.end());
    std::string text;
    for (const std::string& name : names) {
        text += name;
        text += '\n';
    }
    return ToolOutcome{ToolStatus::Ok, text, false,
                       Utf8::from_path(decision.resolved) + " " + std::to_string(names.size()) + " entries"};
}

ToolSpec ReadFileTool::spec() const {
    return ToolSpec{
        "read_file",
        "Read a UTF-8 text file. Maximum size is 65536 bytes.",
        R"({"type":"object","properties":{)" + kPathProperty + R"(},"required":["path"]})",
    };
}

ToolOutcome ReadFileTool::execute(const nlohmann::json& arguments) const {
    const AccessDecision decision =
        check_path(context_, ToolArguments::required_string(arguments, "path"), AccessKind::Read);
    if (!decision.allowed) {
        return denied(decision.reason);
    }
    std::error_code error_code;
    if (!std::filesystem::is_regular_file(decision.resolved, error_code)) {
        return error("not a file: " + Utf8::from_path(decision.resolved));
    }
    return read_limited(decision.resolved);
}

ToolSpec WriteFileTool::spec() const {
    return ToolSpec{
        "write_file",
        "Write a UTF-8 text file, replacing the previous contents. Creates parent directories.",
        R"({"type":"object","properties":{)" + kPathProperty +
            R"(,"content":{"type":"string"}},"required":["path","content"]})",
    };
}

ToolOutcome WriteFileTool::execute(const nlohmann::json& arguments) const {
    const std::string content = ToolArguments::required_string(arguments, "content");
    if (content.size() > kMaxFileBytes) {
        return error("content larger than " + std::to_string(kMaxFileBytes) + " bytes");
    }
    if (has_nul(content) || !Utf8::is_valid(content)) {
        return error("content is not valid UTF-8 text");
    }
    const AccessDecision decision =
        check_path(context_, ToolArguments::required_string(arguments, "path"), AccessKind::Write);
    if (!decision.allowed) {
        return denied(decision.reason);
    }
    Utf8::write_file(decision.resolved, content);
    return ToolOutcome{ToolStatus::Ok, "ok: wrote " + Utf8::from_path(decision.resolved),
                       decision.in_content_repo, "wrote " + Utf8::from_path(decision.resolved)};
}

ToolSpec ReplaceInFileTool::spec() const {
    return ToolSpec{
        "replace_in_file",
        "Replace the unique occurrence of find with replace in a UTF-8 text file. Fails if find is missing or appears more than once.",
        R"({"type":"object","properties":{)" + kPathProperty +
            R"(,"find":{"type":"string"},"replace":{"type":"string"}},"required":["path","find","replace"]})",
    };
}

ToolOutcome ReplaceInFileTool::execute(const nlohmann::json& arguments) const {
    const std::string find = ToolArguments::required_string(arguments, "find");
    const std::string replace = ToolArguments::required_string(arguments, "replace");
    if (find.empty()) {
        return error("find is empty");
    }
    const AccessDecision decision =
        check_path(context_, ToolArguments::required_string(arguments, "path"), AccessKind::Write);
    if (!decision.allowed) {
        return denied(decision.reason);
    }
    std::error_code error_code;
    if (!std::filesystem::is_regular_file(decision.resolved, error_code)) {
        return error("not a file: " + Utf8::from_path(decision.resolved));
    }
    const ToolOutcome loaded = read_limited(decision.resolved);
    if (loaded.status != ToolStatus::Ok) {
        return loaded;
    }
    const std::string& original = loaded.result_text;
    const auto first = original.find(find);
    if (first == std::string::npos) {
        return error("find text not found");
    }
    if (original.find(find, first + find.size()) != std::string::npos) {
        return error("find text occurs more than once");
    }
    std::string updated = original;
    updated.replace(first, find.size(), replace);
    if (updated.size() > kMaxFileBytes) {
        return error("result larger than " + std::to_string(kMaxFileBytes) + " bytes");
    }
    Utf8::write_file(decision.resolved, updated);
    return ToolOutcome{ToolStatus::Ok, "ok: replaced in " + Utf8::from_path(decision.resolved),
                       decision.in_content_repo, "replaced in " + Utf8::from_path(decision.resolved)};
}

}  // namespace todo
