#include "prompt/confirm_window.h"

#include <windows.h>

#include <algorithm>
#include <optional>
#include <string>

#include "prompt/window_support.h"
#include "util/todo_error.h"

namespace todo {

namespace {

constexpr wchar_t kClassName[] = L"TodoConfirmWindow";
constexpr COLORREF kBackground = RGB(20, 22, 28);
constexpr COLORREF kText = RGB(235, 235, 235);
constexpr COLORREF kWarning = RGB(253, 186, 116);
constexpr COLORREF kDim = RGB(150, 150, 150);

// Layout in 96-DPI units.
constexpr int kPadding = 10;
constexpr int kTitleHeight = 20;
constexpr int kHintHeight = 18;
constexpr int kGap = 6;
constexpr int kDefaultWidth = 420;
constexpr int kAnchorOffsetY = 40;
constexpr int kMaxDetailHeight = 320;

struct ConfirmState {
    HWND title = nullptr;
    HWND hint = nullptr;
    HBRUSH background = nullptr;
    bool allowed = false;
};

LRESULT CALLBACK window_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    auto* state = reinterpret_cast<ConfirmState*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    switch (message) {
        case WM_NCCREATE: {
            const auto* create = reinterpret_cast<const CREATESTRUCTW*>(lparam);
            SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create->lpCreateParams));
            break;
        }
        case WM_CTLCOLORSTATIC:
            if (state) {
                const HWND control = reinterpret_cast<HWND>(lparam);
                const COLORREF color = control == state->title ? kWarning : control == state->hint ? kDim : kText;
                SetTextColor(reinterpret_cast<HDC>(wparam), color);
                SetBkColor(reinterpret_cast<HDC>(wparam), kBackground);
                return reinterpret_cast<LRESULT>(state->background);
            }
            break;
        case WM_NCHITTEST: {
            const LRESULT hit = DefWindowProcW(window, message, wparam, lparam);
            return hit == HTCLIENT ? HTCAPTION : hit;
        }
        case WM_CLOSE:
            DestroyWindow(window);
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        default:
            break;
    }
    return DefWindowProcW(window, message, wparam, lparam);
}

void register_class(HINSTANCE instance, HBRUSH background) {
    WNDCLASSEXW window_class{};
    window_class.cbSize = sizeof(window_class);
    window_class.lpfnWndProc = window_proc;
    window_class.hInstance = instance;
    window_class.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    window_class.hbrBackground = background;
    window_class.lpszClassName = kClassName;
    if (RegisterClassExW(&window_class) == 0 && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        throw TodoError("confirm: RegisterClassExW failed");
    }
}

RECT cursor_work_area() {
    POINT cursor{};
    GetCursorPos(&cursor);
    MONITORINFO info{};
    info.cbSize = sizeof(info);
    GetMonitorInfoW(MonitorFromPoint(cursor, MONITOR_DEFAULTTOPRIMARY), &info);
    return info.rcWork;
}

int measure_height(HFONT font, const std::wstring& text, int width) {
    const HDC dc = GetDC(nullptr);
    const HGDIOBJ previous = SelectObject(dc, font);
    RECT rect{0, 0, width, 0};
    DrawTextW(dc, text.c_str(), static_cast<int>(text.size()), &rect, DT_CALCRECT | DT_WORDBREAK | DT_NOPREFIX);
    SelectObject(dc, previous);
    ReleaseDC(nullptr, dc);
    return rect.bottom;
}

}  // namespace

bool ConfirmWindow::run(const ConfirmRequest& request) const {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    const HINSTANCE instance = GetModuleHandleW(nullptr);
    const std::optional<RECT> anchor =
        request.anchor_title.empty() ? std::nullopt : WindowSupport::find_skin_rect(request.anchor_title);
    const RECT area = cursor_work_area();

    ConfirmState state;
    state.background = CreateSolidBrush(kBackground);
    register_class(instance, state.background);

    // Created on the target monitor first so GetDpiForWindow reports that monitor's DPI.
    const int origin_x = anchor ? anchor->left : area.left;
    const int origin_y = anchor ? anchor->top : area.top;
    const HWND window = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TOOLWINDOW, kClassName, L"ToDo",
                                        WS_POPUP | WS_BORDER, origin_x, origin_y, 1, 1, nullptr, nullptr,
                                        instance, &state);
    if (!window) {
        DeleteObject(state.background);
        throw TodoError("confirm: CreateWindowExW failed");
    }

    const UINT dpi = GetDpiForWindow(window);
    const auto scale = [dpi](int value) { return WindowSupport::scale(value, dpi); };
    const int width = anchor ? anchor->right - anchor->left : scale(kDefaultWidth);
    const int padding = scale(kPadding);
    const int inner_width = width - 2 * padding;
    const HFONT title_font = WindowSupport::create_font(10, dpi, FW_BOLD);
    const HFONT detail_font = WindowSupport::create_font(9, dpi, FW_NORMAL);
    const int detail_height = std::min(measure_height(detail_font, request.detail, inner_width), scale(kMaxDetailHeight));
    const int detail_top = padding + scale(kTitleHeight) + scale(kGap);
    const int hint_top = detail_top + detail_height + scale(kGap);
    const int height = hint_top + scale(kHintHeight) + padding;
    const int x = anchor ? anchor->left : area.left + (area.right - area.left - width) / 2;
    const int y = anchor ? anchor->top + scale(kAnchorOffsetY) : area.top + (area.bottom - area.top - height) / 2;
    SetWindowPos(window, HWND_TOPMOST, x, y, width, height, SWP_NOACTIVATE);

    state.title = CreateWindowExW(0, L"STATIC", request.title.c_str(), WS_CHILD | WS_VISIBLE | SS_LEFT | SS_NOPREFIX,
                                  padding, padding, inner_width, scale(kTitleHeight), window, nullptr, instance,
                                  nullptr);
    const HWND detail = CreateWindowExW(0, L"STATIC", request.detail.c_str(),
                                        WS_CHILD | WS_VISIBLE | SS_LEFT | SS_NOPREFIX | SS_EDITCONTROL, padding,
                                        detail_top, inner_width, detail_height, window, nullptr, instance, nullptr);
    state.hint = CreateWindowExW(0, L"STATIC", L"Enter: allow    Esc: deny", WS_CHILD | WS_VISIBLE | SS_RIGHT,
                                 padding, hint_top, inner_width, scale(kHintHeight), window, nullptr, instance,
                                 nullptr);
    SendMessageW(state.title, WM_SETFONT, reinterpret_cast<WPARAM>(title_font), TRUE);
    SendMessageW(detail, WM_SETFONT, reinterpret_cast<WPARAM>(detail_font), TRUE);
    SendMessageW(state.hint, WM_SETFONT, reinterpret_cast<WPARAM>(detail_font), TRUE);

    WindowSupport::take_foreground(window, window);

    MSG message;
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        if (message.message == WM_KEYDOWN && (message.wParam == VK_RETURN || message.wParam == VK_ESCAPE)) {
            state.allowed = message.wParam == VK_RETURN;
            DestroyWindow(window);
            continue;
        }
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    DeleteObject(title_font);
    DeleteObject(detail_font);
    DeleteObject(state.background);
    return state.allowed;
}

}  // namespace todo
