#pragma once

#include <string>

namespace todo {

struct ConfirmRequest {
    std::wstring title;
    std::wstring detail;        // shown verbatim; callers keep it short
    std::wstring anchor_title;  // Rainmeter skin window title; empty or missing: centered on the cursor's monitor
};

// Topmost native window. Enter allows; Esc or closing the window denies.
class ConfirmWindow {
public:
    bool run(const ConfirmRequest& request) const;
};

}  // namespace todo
