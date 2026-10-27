#pragma once

#include <optional>
#include <string>

#include "prompt/prompt_theme.h"

namespace todo {

struct PromptRequest {
    std::wstring caption;       // shown above the edit box
    std::wstring anchor_title;  // title of the Rainmeter skin window (its full .ini path)
    int offset_x = 0;           // from the skin's top-left corner, in skin units (96 DPI)
    int offset_y = 0;
    PromptTheme theme;          // colors of the skin's current style
    // Docked: right below the skin window at its width, following it as it moves or resizes, not draggable;
    // closes as cancelled when the skin window goes away. The offsets are not used.
    bool dock_below = false;
};

struct PromptResult {
    std::optional<std::wstring> text;  // nullopt: cancelled
    int offset_x = 0;                  // final position relative to the skin, in skin units
    int offset_y = 0;
};

// A topmost single-line input window placed relative to a Rainmeter skin window and drawn in that skin's
// colors, with rounded corners and an accent outline (Windows 11). It takes the foreground, can be dragged,
// and stays open until Enter or Esc; losing focus does not close it.
class PromptWindow {
public:
    // Throws TodoError if the anchor skin window does not exist.
    PromptResult run(const PromptRequest& request) const;
};

}  // namespace todo
