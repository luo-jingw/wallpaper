#include "util/text.h"

#include <algorithm>
#include <charconv>
#include <sstream>
#include <system_error>

namespace todo {

std::string Text::trim(const std::string& text) {
    const auto begin = text.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) {
        return {};
    }
    const auto end = text.find_last_not_of(" \t\r\n");
    return text.substr(begin, end - begin + 1);
}

std::vector<std::string> Text::split_lines(const std::string& text) {
    std::vector<std::string> lines;
    std::istringstream stream(text);
    std::string line;
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        lines.push_back(line);
    }
    return lines;
}

std::vector<std::string> Text::split_whitespace(const std::string& text) {
    std::vector<std::string> parts;
    std::istringstream stream(text);
    std::string part;
    while (stream >> part) {
        parts.push_back(part);
    }
    return parts;
}

bool Text::starts_with(const std::string& text, const std::string& prefix) {
    return text.rfind(prefix, 0) == 0;
}

std::string Text::truncate_code_points(const std::string& utf8, size_t max_code_points) {
    size_t end = 0;
    for (size_t count = 0; end < utf8.size() && count < max_code_points; ++count) {
        const unsigned char lead = static_cast<unsigned char>(utf8[end]);
        size_t length = 1;
        if ((lead & 0xE0) == 0xC0) length = 2;
        else if ((lead & 0xF0) == 0xE0) length = 3;
        else if ((lead & 0xF8) == 0xF0) length = 4;
        end = std::min(utf8.size(), end + length);
    }
    return utf8.substr(0, end);
}

std::string Text::single_line(const std::string& text) {
    std::string result = text;
    for (char& c : result) {
        if (c == '\r' || c == '\n' || c == '\t') {
            c = ' ';
        }
    }
    return trim(result);
}

std::optional<int> Text::parse_positive_int(const std::string& text) {
    int value = 0;
    const char* const end = text.data() + text.size();
    const auto [last, error] = std::from_chars(text.data(), end, value);
    if (text.empty() || text[0] == '-' || text[0] == '+' || error != std::errc() || last != end || value <= 0) {
        return std::nullopt;
    }
    return value;
}

}  // namespace todo
