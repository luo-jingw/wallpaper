// Rainmeter plugin: the mouse cursor position and a high-resolution clock.
// The number value is the cursor X; the string value is "X,Y,MS": the cursor position and the milliseconds
// since the measure was created (QueryPerformanceCounter), so a script can animate by real elapsed time.
// Coordinates come from GetCursorPos inside the Rainmeter process, so they use the same (DPI-virtualized)
// coordinate space as #CURRENTCONFIGX#.
// Uses the plain plugin exports only; no Rainmeter SDK functions are needed.

#include <windows.h>

#include <cstdio>

namespace {

struct CursorMeasure {
    POINT position{0, 0};
    LARGE_INTEGER start{};
    LARGE_INTEGER frequency{};
    wchar_t text[64] = L"0,0,0";
};

}  // namespace

extern "C" {

__declspec(dllexport) void Initialize(void** data, void* /*rm*/) {
    auto* measure = new CursorMeasure();
    QueryPerformanceFrequency(&measure->frequency);
    QueryPerformanceCounter(&measure->start);
    *data = measure;
}

__declspec(dllexport) void Reload(void* /*data*/, void* /*rm*/, double* /*max_value*/) {}

__declspec(dllexport) double Update(void* data) {
    auto* measure = static_cast<CursorMeasure*>(data);
    GetCursorPos(&measure->position);  // keeps the last position if the desktop is locked
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    const double ms = 1000.0 * static_cast<double>(now.QuadPart - measure->start.QuadPart) /
                      static_cast<double>(measure->frequency.QuadPart);
    swprintf_s(measure->text, L"%ld,%ld,%.3f", measure->position.x, measure->position.y, ms);
    return measure->position.x;
}

__declspec(dllexport) LPCWSTR GetString(void* data) {
    return static_cast<CursorMeasure*>(data)->text;
}

__declspec(dllexport) void Finalize(void* data) {
    delete static_cast<CursorMeasure*>(data);
}

}  // extern "C"
