#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace todo {

struct ProcessResult {
    int exit_code;
    std::string stdout_utf8;
    std::string stderr_utf8;
};

// Starts a child process without a console window and captures its output.
class ProcessRunner {
public:
    // Resolves a bare executable name (e.g. L"git.exe") through PATH.
    std::filesystem::path find_executable(const std::wstring& name) const;

    ProcessResult run(const std::filesystem::path& executable, const std::vector<std::wstring>& args,
                      const std::filesystem::path& working_dir) const;

private:
    static std::wstring quote(const std::wstring& arg);
};

}  // namespace todo
