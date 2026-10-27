#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "util/date.h"

namespace todo {

struct Command {
    std::filesystem::path config_file;
    std::optional<Date> today;
    std::string verb;
    std::vector<std::string> positionals;
    std::optional<Date> date;
    std::optional<std::string> parent;
    bool with_tasks = false;
    std::optional<std::string> skill;
    std::optional<std::string> chat;
    std::optional<std::string> anchor;
    std::optional<int> offset_x;
    std::optional<int> offset_y;
    // prompt: the skin style's colors, "R,G,B[,A]" (ColorBg, ColorPanel, ColorText, ColorDim, ColorAccent).
    std::optional<std::string> color_bg;
    std::optional<std::string> color_panel;
    std::optional<std::string> color_text;
    std::optional<std::string> color_dim;
    std::optional<std::string> color_accent;
    bool dock_below = false;  // prompt: docked below the anchor skin, following it
};

}  // namespace todo
