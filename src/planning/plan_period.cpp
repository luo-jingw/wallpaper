#include "planning/plan_period.h"

#include <optional>

#include "util/todo_error.h"

namespace todo {

namespace {

bool all_digits(const std::string& text) {
    if (text.empty()) {
        return false;
    }
    for (const char c : text) {
        if (c < '0' || c > '9') {
            return false;
        }
    }
    return true;
}

}  // namespace

std::string PlanPeriod::of(TaskLevel scope, const Date& date) {
    if (scope == TaskLevel::Day) {
        return date.to_string();
    }
    if (scope == TaskLevel::Week) {
        return date.iso_week().to_string();
    }
    throw TodoError("plan scope must be day or week");
}

bool PlanPeriod::is_week(const std::string& period) {
    return period.size() == 8 && period[4] == '-' && period[5] == 'W' && all_digits(period.substr(0, 4)) &&
           all_digits(period.substr(6, 2));
}

Date PlanPeriod::anchor(const std::string& period, const Date& today) {
    if (const std::optional<Date> day = Date::parse(period)) {
        return *day;
    }
    if (!is_week(period)) {
        throw TodoError("period must be YYYY-MM-DD or YYYY-Www, got: " + period);
    }
    const IsoWeek week{std::stoi(period.substr(0, 4)), std::stoi(period.substr(6, 2))};
    if (today.iso_week() == week) {
        return today;
    }
    // January 4th is always in ISO week 1.
    const Date jan4(week.year, 1, 4);
    const Date monday = jan4.add_days(1 - jan4.weekday_iso() + 7 * (week.week - 1));
    if (!(monday.iso_week() == week)) {
        throw TodoError("no such ISO week: " + period);
    }
    return monday;
}

}  // namespace todo
