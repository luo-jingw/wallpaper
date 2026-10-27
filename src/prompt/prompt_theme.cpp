#include "prompt/prompt_theme.h"

#include <array>
#include <sstream>
#include <string>

#include "util/todo_error.h"

namespace todo {

namespace {

struct SkinColor {
    int r = 0;
    int g = 0;
    int b = 0;
    int a = 255;
};

SkinColor parse_color(const std::string& text) {
    std::array<int, 4> parts{0, 0, 0, 255};
    std::istringstream stream(text);
    std::string item;
    size_t count = 0;
    while (std::getline(stream, item, ',')) {
        if (count == parts.size() || item.empty() || item.find_first_not_of("0123456789 ") != std::string::npos) {
            throw TodoError("invalid skin color: " + text);
        }
        const int value = std::stoi(item);
        if (value > 255) {
            throw TodoError("invalid skin color: " + text);
        }
        parts[count++] = value;
    }
    if (count != 3 && count != 4) {
        throw TodoError("invalid skin color: " + text);
    }
    return SkinColor{parts[0], parts[1], parts[2], parts[3]};
}

COLORREF opaque(const SkinColor& color) {
    return RGB(color.r, color.g, color.b);
}

// `over` drawn with its alpha on the opaque `under`.
COLORREF composite(const SkinColor& over, const SkinColor& under) {
    const auto mix = [&](int top, int bottom) { return (top * over.a + bottom * (255 - over.a) + 127) / 255; };
    return RGB(mix(over.r, under.r), mix(over.g, under.g), mix(over.b, under.b));
}

}  // namespace

PromptTheme PromptTheme::from_skin_colors(const std::string& background, const std::string& panel,
                                          const std::string& text, const std::string& dim,
                                          const std::string& accent) {
    const SkinColor bg = parse_color(background);
    PromptTheme theme;
    theme.background = opaque(bg);
    theme.field = composite(parse_color(panel), bg);
    theme.text = opaque(parse_color(text));
    theme.dim = opaque(parse_color(dim));
    theme.border = opaque(parse_color(accent));
    return theme;
}

}  // namespace todo
