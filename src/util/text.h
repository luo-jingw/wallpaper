#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace todo {

class Text {
public:
    static std::string trim(const std::string& text);
    static std::vector<std::string> split_lines(const std::string& text);  // strips trailing '\r'
    static std::vector<std::string> split_whitespace(const std::string& text);
    static bool starts_with(const std::string& text, const std::string& prefix);
    static std::string single_line(const std::string& text);  // CR, LF, TAB become spaces; result trimmed
    static std::string truncate_code_points(const std::string& utf8, size_t max_code_points);
    static std::optional<int> parse_positive_int(const std::string& text);  // whole string must be digits, > 0
};

}  // namespace todo
