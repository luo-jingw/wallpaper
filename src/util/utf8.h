#pragma once

#include <filesystem>
#include <string>

namespace todo {

class Utf8 {
public:
    static std::string from_wide(const std::wstring& wide);
    static std::wstring to_wide(const std::string& utf8);
    static bool is_valid(const std::string& bytes);
    static std::string from_path(const std::filesystem::path& path);
    static std::filesystem::path to_path(const std::string& utf8);
    static std::string read_file(const std::filesystem::path& path);
    static void write_file(const std::filesystem::path& path, const std::string& utf8);
};

}  // namespace todo
