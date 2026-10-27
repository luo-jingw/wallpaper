#include "prompt/window_support.h"

namespace todo {

int WindowSupport::scale(int value_at_96_dpi, UINT dpi) {
    return MulDiv(value_at_96_dpi, static_cast<int>(dpi), 96);
}

HFONT WindowSupport::create_font(int points, UINT dpi, int weight) {
    return CreateFontW(-MulDiv(points, static_cast<int>(dpi), 72), 0, 0, 0, weight, FALSE, FALSE, FALSE,
                       DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                       DEFAULT_PITCH, L"Microsoft YaHei UI");
}

std::optional<RECT> WindowSupport::find_skin_rect(const std::wstring& skin_title) {
    const HWND skin = FindWindowW(L"RainmeterMeterWindow", skin_title.c_str());
    RECT rect{};
    if (!skin || !GetWindowRect(skin, &rect)) {
        return std::nullopt;
    }
    return rect;
}

void WindowSupport::take_foreground(HWND window, HWND focus) {
    const HWND foreground = GetForegroundWindow();
    const DWORD foreground_thread = foreground ? GetWindowThreadProcessId(foreground, nullptr) : 0;
    const DWORD self_thread = GetCurrentThreadId();
    const bool attached = foreground_thread != 0 && foreground_thread != self_thread &&
                          AttachThreadInput(self_thread, foreground_thread, TRUE);
    ShowWindow(window, SW_SHOW);
    BringWindowToTop(window);
    SetForegroundWindow(window);
    SetFocus(focus);
    if (attached) {
        AttachThreadInput(self_thread, foreground_thread, FALSE);
    }
}

}  // namespace todo
