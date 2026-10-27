#pragma once

#include <string>

#include "domain/task.h"
#include "util/date.h"

namespace todo {

// A plan period is a day ("YYYY-MM-DD") or an ISO week ("YYYY-Www").
class PlanPeriod {
public:
    static std::string of(TaskLevel scope, const Date& date);  // scope: Day or Week
    static bool is_week(const std::string& period);
    // The date whose task files a plan reads and changes: the day itself, or for a week, today when today is
    // inside it and its Monday otherwise. Throws TodoError for a malformed period.
    static Date anchor(const std::string& period, const Date& today);
};

}  // namespace todo
