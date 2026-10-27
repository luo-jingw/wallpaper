#include "vcs/git_client.h"

#include <utility>

#include "util/text.h"
#include "util/todo_error.h"
#include "util/utf8.h"

namespace todo {

GitClient::GitClient(const ProcessRunner& runner, std::filesystem::path repo_dir)
    : runner_(runner), repo_dir_(std::move(repo_dir)), git_exe_(runner.find_executable(L"git.exe")) {}

void GitClient::ensure_repository() const {
    if (std::filesystem::exists(repo_dir_ / ".git")) {
        return;
    }
    std::filesystem::create_directories(repo_dir_);
    git_checked({L"init", L"-q", L"-b", L"main"});
    Utf8::write_file(repo_dir_ / ".gitattributes", "* text=auto eol=lf\n");
}

std::optional<std::string> GitClient::commit_all(const std::string& message_utf8) const {
    git_checked({L"add", L"-A"});
    const ProcessResult diff = git({L"diff", L"--cached", L"--quiet"});
    if (diff.exit_code == 0) {
        return std::nullopt;
    }
    git_checked({L"commit", L"-q", L"-m", Utf8::to_wide(message_utf8)});
    return Text::trim(git_checked({L"rev-parse", L"--short", L"HEAD"}).stdout_utf8);
}

ProcessResult GitClient::git(const std::vector<std::wstring>& args) const {
    std::vector<std::wstring> full_args = {L"-c", L"core.quotepath=false"};
    full_args.insert(full_args.end(), args.begin(), args.end());
    return runner_.run(git_exe_, full_args, repo_dir_);
}

ProcessResult GitClient::git_checked(const std::vector<std::wstring>& args) const {
    ProcessResult result = git(args);
    if (result.exit_code != 0) {
        const std::string command = args.empty() ? "" : Utf8::from_wide(args.front());
        throw TodoError("git " + command + " failed: " + Text::single_line(result.stderr_utf8));
    }
    return result;
}

}  // namespace todo
