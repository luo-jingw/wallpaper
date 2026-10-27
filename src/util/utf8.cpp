#include "util/utf8.h"

#include <windows.h>

#include <fstream>
#include <iterator>

#include "util/todo_error.h"

namespace todo {

std::string Utf8::from_wide(const std::wstring& wide) {
    if (wide.empty()) {
        return {};
    }
    const int size = WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()),
                                         nullptr, 0, nullptr, nullptr);
    std::string result(static_cast<size_t>(size), '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), result.data(), size,
                        nullptr, nullptr);
    return result;
}

std::wstring Utf8::to_wide(const std::string& utf8) {
    if (utf8.empty()) {
        return {};
    }
    const int size = MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), nullptr, 0);
    std::wstring result(static_cast<size_t>(size), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), result.data(), size);
    return result;
}

bool Utf8::is_valid(const std::string& bytes) {
    return bytes.empty() || MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, bytes.data(),
                                                static_cast<int>(bytes.size()), nullptr, 0) > 0;
}

std::string Utf8::from_path(const std::filesystem::path& path) {
    return from_wide(path.wstring());
}

std::filesystem::path Utf8::to_path(const std::string& utf8) {
    return std::filesystem::path(to_wide(utf8));
}

std::string Utf8::read_file(const std::filesystem::path& path) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        throw TodoError("cannot read file: " + from_path(path));
    }
    std::string content((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
    if (content.size() >= 3 && content.compare(0, 3, "\xEF\xBB\xBF") == 0) {
        content.erase(0, 3);
    }
    return content;
}

void Utf8::write_file(const std::filesystem::path& path, const std::string& utf8) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    if (!stream) {
        throw TodoError("cannot write file: " + from_path(path));
    }
    stream.write(utf8.data(), static_cast<std::streamsize>(utf8.size()));
}

}  // namespace todo
