#pragma once

#include <string>

namespace todo {

// The plan JSON shape and scheduling rules given to the model, shared by plan day|week and propose_plan.
class PlanFormat {
public:
    static std::string example(bool week);
    static std::string rules(bool week);
    // Both time formats, for a caller that may write either kind of plan.
    static std::string time_formats();
};

}  // namespace todo
