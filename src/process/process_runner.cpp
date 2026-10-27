#include "process/process_runner.h"

#include <windows.h>

#include <thread>

#include "util/todo_error.h"
#include "util/utf8.h"

namespace todo {

namespace {

class PipeReader {
public:
    explicit PipeReader(HANDLE handle) : handle_(handle) {}

    void read_all() {
        char buffer[4096];
        DWORD read = 0;
        while (ReadFile(handle_, buffer, sizeof(buffer), &read, nullptr) && read > 0) {
            data_.append(buffer, read);
        }
    }

    const std::string& data() const { return data_; }

private:
    HANDLE handle_;
    std::string data_;
};

void create_pipe(HANDLE& read_end, HANDLE& write_end) {
    SECURITY_ATTRIBUTES attributes{sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE};
    if (!CreatePipe(&read_end, &write_end, &attributes, 0)) {
        throw TodoError("CreatePipe failed");
    }
    SetHandleInformation(read_end, HANDLE_FLAG_INHERIT, 0);
}

}  // namespace

std::filesystem::path ProcessRunner::find_executable(const std::wstring& name) const {
    wchar_t buffer[MAX_PATH];
    const DWORD length = SearchPathW(nullptr, name.c_str(), nullptr, MAX_PATH, buffer, nullptr);
    if (length == 0 || length >= MAX_PATH) {
        throw TodoError("executable not found on PATH: " + Utf8::from_wide(name));
    }
    return std::filesystem::path(buffer);
}

ProcessResult ProcessRunner::run(const std::filesystem::path& executable, const std::vector<std::wstring>& args,
                                 const std::filesystem::path& working_dir) const {
    std::wstring command_line = quote(executable.wstring());
    for (const std::wstring& arg : args) {
        command_line += L' ';
        command_line += quote(arg);
    }

    HANDLE stdout_read = nullptr;
    HANDLE stdout_write = nullptr;
    HANDLE stderr_read = nullptr;
    HANDLE stderr_write = nullptr;
    create_pipe(stdout_read, stdout_write);
    create_pipe(stderr_read, stderr_write);

    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdInput = nullptr;
    startup.hStdOutput = stdout_write;
    startup.hStdError = stderr_write;

    PROCESS_INFORMATION process{};
    const BOOL created = CreateProcessW(executable.c_str(), command_line.data(), nullptr, nullptr, TRUE,
                                        CREATE_NO_WINDOW, nullptr, working_dir.c_str(), &startup, &process);
    CloseHandle(stdout_write);
    CloseHandle(stderr_write);
    if (!created) {
        CloseHandle(stdout_read);
        CloseHandle(stderr_read);
        throw TodoError("CreateProcessW failed for " + Utf8::from_path(executable) +
                        " (error " + std::to_string(GetLastError()) + ")");
    }

    PipeReader out(stdout_read);
    PipeReader err(stderr_read);
    std::thread err_thread([&err] { err.read_all(); });
    out.read_all();
    err_thread.join();

    WaitForSingleObject(process.hProcess, INFINITE);
    DWORD exit_code = 0;
    GetExitCodeProcess(process.hProcess, &exit_code);
    CloseHandle(process.hProcess);
    CloseHandle(process.hThread);
    CloseHandle(stdout_read);
    CloseHandle(stderr_read);
    return ProcessResult{static_cast<int>(exit_code), out.data(), err.data()};
}

// Quoting rules of CommandLineToArgvW / the MSVC runtime.
std::wstring ProcessRunner::quote(const std::wstring& arg) {
    if (!arg.empty() && arg.find_first_of(L" \t\n\v\"") == std::wstring::npos) {
        return arg;
    }
    std::wstring quoted = L"\"";
    size_t backslashes = 0;
    for (const wchar_t c : arg) {
        if (c == L'\\') {
            ++backslashes;
        } else if (c == L'"') {
            quoted.append(backslashes * 2 + 1, L'\\');
            quoted += c;
            backslashes = 0;
        } else {
            quoted.append(backslashes, L'\\');
            quoted += c;
            backslashes = 0;
        }
    }
    quoted.append(backslashes * 2, L'\\');
    quoted += L'"';
    return quoted;
}

}  // namespace todo
