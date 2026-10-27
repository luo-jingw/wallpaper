// Rainmeter plugin: desktop state for the mascot.
// The string value is "VISIBLE,LEFT,TOP,RIGHT,BOTTOM,IDLE_MS,FULLSCREEN":
// - VISIBLE (0/1) and the rectangle of the watched window, a window of this Rainmeter process whose title is
//   set by the bang "Watch <title>" (a skin window's title is its .ini path); 0,0,0,0,0 while it is missing;
// - IDLE_MS: milliseconds since the last keyboard or mouse input in the session (GetLastInputInfo);
// - FULLSCREEN (0/1): the foreground window has no title bar and covers its whole monitor (a video, a game,
//   a presentation); the desktop and the taskbar do not count.
// Coordinates come from inside the Rainmeter process, in the same space as #CURRENTCONFIGX#.
// The number value is IDLE_MS. Uses the plain plugin exports only; no Rainmeter SDK functions are needed.

#include <windows.h>

#include <cstdio>
#include <string>

namespace {

struct DesktopMeasure {
    std::wstring watched_title;
    wchar_t text[128] = L"0,0,0,0,0,0,0";
};

bool is_shell_window(HWND window) {
    wchar_t name[64] = L"";
    GetClassNameW(window, name, 64);
    const std::wstring cls = name;
    return cls == L"Progman" || cls == L"WorkerW" || cls == L"Shell_TrayWnd" || cls == L"Shell_SecondaryTrayWnd";
}

bool foreground_is_fullscreen() {
    HWND window = GetForegroundWindow();
    if (window == nullptr || is_shell_window(window) || !IsWindowVisible(window)) {
        return false;
    }
    // A maximized window with a title bar also covers the monitor when the taskbar auto-hides; full-screen
    // videos, games, and browsers (F11) drop the title bar.
    if ((GetWindowLongW(window, GWL_STYLE) & WS_CAPTION) == WS_CAPTION) {
        return false;
    }
    RECT rect;
    if (!GetWindowRect(window, &rect)) {
        return false;
    }
    MONITORINFO monitor{};
    monitor.cbSize = sizeof(monitor);
    if (!GetMonitorInfoW(MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST), &monitor)) {
        return false;
    }
    return rect.left <= monitor.rcMonitor.left && rect.top <= monitor.rcMonitor.top &&
           rect.right >= monitor.rcMonitor.right && rect.bottom >= monitor.rcMonitor.bottom;
}

unsigned long idle_milliseconds() {
    LASTINPUTINFO input{};
    input.cbSize = sizeof(input);
    if (!GetLastInputInfo(&input)) {
        return 0;
    }
    return GetTickCount() - input.dwTime;
}

// The watched window, if it exists and belongs to this process.
HWND find_watched(const std::wstring& title) {
    if (title.empty()) {
        return nullptr;
    }
    HWND window = FindWindowW(nullptr, title.c_str());
    if (window == nullptr) {
        return nullptr;
    }
    DWORD process = 0;
    GetWindowThreadProcessId(window, &process);
    return process == GetCurrentProcessId() ? window : nullptr;
}

}  // namespace

extern "C" {

__declspec(dllexport) void Initialize(void** data, void* /*rm*/) {
    *data = new DesktopMeasure();
}

__declspec(dllexport) void Reload(void* /*data*/, void* /*rm*/, double* /*max_value*/) {}

__declspec(dllexport) double Update(void* data) {
    auto* measure = static_cast<DesktopMeasure*>(data);
    int visible = 0;
    RECT rect{0, 0, 0, 0};
    if (HWND window = find_watched(measure->watched_title)) {
        visible = IsWindowVisible(window) && !IsIconic(window) ? 1 : 0;
        GetWindowRect(window, &rect);
    }
    const unsigned long idle = idle_milliseconds();
    swprintf_s(measure->text, L"%d,%ld,%ld,%ld,%ld,%lu,%d", visible, rect.left, rect.top, rect.right, rect.bottom,
               idle, foreground_is_fullscreen() ? 1 : 0);
    return static_cast<double>(idle);
}

__declspec(dllexport) LPCWSTR GetString(void* data) {
    return static_cast<DesktopMeasure*>(data)->text;
}

// "Watch <title>": the window whose visibility and rectangle are reported.
__declspec(dllexport) void ExecuteBang(void* data, LPCWSTR args) {
    const std::wstring command = args;
    const std::wstring prefix = L"Watch ";
    if (command.compare(0, prefix.size(), prefix) == 0) {
        static_cast<DesktopMeasure*>(data)->watched_title = command.substr(prefix.size());
    }
}

__declspec(dllexport) void Finalize(void* data) {
    delete static_cast<DesktopMeasure*>(data);
}

}  // extern "C"
