#pragma once

#include <windows.h>

#include <optional>
#include <string>

namespace todo {

// Shared mechanics of todo.exe's own popup windows. Callers run per-monitor DPI aware.
class WindowSupport {
public:
    static int scale(int value_at_96_dpi, UINT dpi);
    static HFONT create_font(int points, UINT dpi, int weight);
    // Rainmeter skin windows are not DPI aware; in a per-monitor aware process GetWindowRect
    // returns their physical rectangle, which is the only coordinate space both sides share.
    static std::optional<RECT> find_skin_rect(const std::wstring& skin_title);
    // Windows only lets the foreground thread hand out the foreground; attaching to its input queue
    // makes the window eligible even when todo.exe was started by a background process.
    static void take_foreground(HWND window, HWND focus);
};

}  // namespace todo
