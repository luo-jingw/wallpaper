#include "prompt/prompt_window.h"

#include <windows.h>

#include <dwmapi.h>

#include <optional>
#include <string>

#include "prompt/window_support.h"
#include "util/todo_error.h"

namespace todo {

namespace {

constexpr wchar_t kClassName[] = L"TodoPromptWindow";

// Layout in 96-DPI units; scaled by the DPI of the monitor the window is on.
constexpr int kPadding = 8;
constexpr int kCaptionHeight = 20;
constexpr int kGap = 4;
constexpr int kEditHeight = 28;
constexpr int kFontPoints = 10;
constexpr int kCaptionFontPoints = 9;
constexpr int kDockGap = 4;            // between the skin's bottom edge and a docked window
constexpr UINT_PTR kFollowTimer = 1;
constexpr UINT kFollowMilliseconds = 30;

struct PromptState {
    HWND caption = nullptr;
    HWND edit = nullptr;
    HFONT font = nullptr;
    HFONT caption_font = nullptr;
    HBRUSH background = nullptr;
    HBRUSH edit_background = nullptr;
    PromptTheme theme;
    bool docked = false;
    std::wstring anchor_title;
    int dock_gap = 0;  // device pixels
    std::optional<std::wstring> result;
};

int scale(int value, UINT dpi) {
    return WindowSupport::scale(value, dpi);
}

LRESULT CALLBACK window_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    auto* state = reinterpret_cast<PromptState*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    switch (message) {
        case WM_NCCREATE: {
            const auto* create = reinterpret_cast<const CREATESTRUCTW*>(lparam);
            SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create->lpCreateParams));
            break;
        }
        case WM_CTLCOLORSTATIC:
            if (state) {
                SetTextColor(reinterpret_cast<HDC>(wparam), state->theme.dim);
                SetBkColor(reinterpret_cast<HDC>(wparam), state->theme.background);
                return reinterpret_cast<LRESULT>(state->background);
            }
            break;
        case WM_CTLCOLOREDIT:
            if (state) {
                SetTextColor(reinterpret_cast<HDC>(wparam), state->theme.text);
                SetBkColor(reinterpret_cast<HDC>(wparam), state->theme.field);
                return reinterpret_cast<LRESULT>(state->edit_background);
            }
            break;
        case WM_NCHITTEST: {
            // Everything outside the edit box drags the window, unless it is docked to the skin.
            const LRESULT hit = DefWindowProcW(window, message, wparam, lparam);
            return hit == HTCLIENT && state && !state->docked ? HTCAPTION : hit;
        }
        case WM_TIMER:
            if (state && wparam == kFollowTimer) {
                const std::optional<RECT> anchor = WindowSupport::find_skin_rect(state->anchor_title);
                if (!anchor) {
                    DestroyWindow(window);  // the skin is gone: cancelled
                    return 0;
                }
                RECT current{};
                GetWindowRect(window, &current);
                const int x = anchor->left;
                const int y = anchor->bottom + state->dock_gap;
                const int width = anchor->right - anchor->left;
                if (current.left != x || current.top != y || current.right - current.left != width) {
                    SetWindowPos(window, HWND_TOPMOST, x, y, width, current.bottom - current.top, SWP_NOACTIVATE);
                }
                return 0;
            }
            break;
        case WM_ACTIVATE:
            if (state && LOWORD(wparam) != WA_INACTIVE) {
                SetFocus(state->edit);
            }
            return 0;
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
        throw TodoError("prompt: RegisterClassExW failed");
    }
}

// Rounded corners and an outline in the accent color; Windows 10 ignores both attributes.
void apply_frame(HWND window, COLORREF border) {
    const DWM_WINDOW_CORNER_PREFERENCE corners = DWMWCP_ROUND;
    DwmSetWindowAttribute(window, DWMWA_WINDOW_CORNER_PREFERENCE, &corners, sizeof(corners));
    DwmSetWindowAttribute(window, DWMWA_BORDER_COLOR, &border, sizeof(border));
}

RECT anchor_rect(const std::wstring& anchor_title) {
    const std::optional<RECT> rect = WindowSupport::find_skin_rect(anchor_title);
    if (!rect) {
        throw TodoError("prompt: skin window not found");
    }
    return *rect;
}

}  // namespace

PromptResult PromptWindow::run(const PromptRequest& request) const {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    const HINSTANCE instance = GetModuleHandleW(nullptr);
    const RECT anchor = anchor_rect(request.anchor_title);
    const int width = anchor.right - anchor.left;

    PromptState state;
    state.theme = request.theme;
    state.docked = request.dock_below;
    state.anchor_title = request.anchor_title;
    state.background = CreateSolidBrush(request.theme.background);
    state.edit_background = CreateSolidBrush(request.theme.field);
    register_class(instance, state.background);

    // Created at the skin's corner first so GetDpiForWindow reports that monitor's DPI.
    const HWND window = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TOOLWINDOW, kClassName, L"ToDo",
                                        WS_POPUP, anchor.left, anchor.top, width, 1, nullptr, nullptr,
                                        instance, &state);
    if (!window) {
        throw TodoError("prompt: CreateWindowExW failed");
    }
    apply_frame(window, request.theme.border);

    const UINT dpi = GetDpiForWindow(window);
    const int padding = scale(kPadding, dpi);
    const int inner_width = width - 2 * padding;
    const int caption_height = scale(kCaptionHeight, dpi);
    const int edit_height = scale(kEditHeight, dpi);
    const int edit_top = padding + caption_height + scale(kGap, dpi);
    const int height = edit_top + edit_height + padding;
    state.dock_gap = scale(kDockGap, dpi);
    if (request.dock_below) {
        SetWindowPos(window, HWND_TOPMOST, anchor.left, anchor.bottom + state.dock_gap, width, height, SWP_NOACTIVATE);
        SetTimer(window, kFollowTimer, kFollowMilliseconds, nullptr);
    } else {
        SetWindowPos(window, HWND_TOPMOST, anchor.left + scale(request.offset_x, dpi),
                     anchor.top + scale(request.offset_y, dpi), width, height, SWP_NOACTIVATE);
    }

    state.font = WindowSupport::create_font(kFontPoints, dpi, FW_NORMAL);
    state.caption_font = WindowSupport::create_font(kCaptionFontPoints, dpi, FW_NORMAL);
    state.caption = CreateWindowExW(0, L"STATIC", request.caption.c_str(), WS_CHILD | WS_VISIBLE | SS_LEFT,
                                    padding, padding, inner_width, caption_height, window, nullptr, instance,
                                    nullptr);
    state.edit = CreateWindowExW(0, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, padding, edit_top,
                                 inner_width, edit_height, window, nullptr, instance, nullptr);
    SendMessageW(state.caption, WM_SETFONT, reinterpret_cast<WPARAM>(state.caption_font), TRUE);
    SendMessageW(state.edit, WM_SETFONT, reinterpret_cast<WPARAM>(state.font), TRUE);

    WindowSupport::take_foreground(window, state.edit);

    PromptResult result;
    MSG message;
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        // While an IME composition is active, Enter and Esc arrive as VK_PROCESSKEY and reach the IME.
        if (message.message == WM_KEYDOWN && (message.wParam == VK_RETURN || message.wParam == VK_ESCAPE)) {
            if (message.wParam == VK_RETURN) {
                const int length = GetWindowTextLengthW(state.edit);
                std::wstring text(static_cast<size_t>(length) + 1, L'\0');
                GetWindowTextW(state.edit, text.data(), length + 1);
                text.resize(static_cast<size_t>(length));
                state.result = text;
            }
            RECT final_rect{};
            GetWindowRect(window, &final_rect);
            const RECT final_anchor = anchor_rect(request.anchor_title);
            result.offset_x = MulDiv(final_rect.left - final_anchor.left, 96, static_cast<int>(dpi));
            result.offset_y = MulDiv(final_rect.top - final_anchor.top, 96, static_cast<int>(dpi));
            DestroyWindow(window);
            continue;
        }
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    DeleteObject(state.font);
    DeleteObject(state.caption_font);
    DeleteObject(state.background);
    DeleteObject(state.edit_background);
    result.text = state.result;
    return result;
}

}  // namespace todo
