#pragma once

#include <windows.h>

#include <string>

namespace todo {

// Colors of the prompt window, taken from the skin's current style (style-{a,b,c}.inc) so the window
// matches the skin it opens over.
struct PromptTheme {
    COLORREF background = 0;  // ColorBg without its alpha
    COLORREF field = 0;       // ColorPanel composited over ColorBg: the edit box
    COLORREF text = 0;        // ColorText: typed text
    COLORREF dim = 0;         // ColorDim: the caption
    COLORREF border = 0;      // ColorAccent: the window outline

    // Each argument is a skin color "R,G,B" or "R,G,B,A" (0-255). Throws TodoError on a malformed color.
    static PromptTheme from_skin_colors(const std::string& background, const std::string& panel,
                                        const std::string& text, const std::string& dim,
                                        const std::string& accent);
};

}  // namespace todo
