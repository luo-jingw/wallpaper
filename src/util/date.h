#pragma once

#include <optional>
#include <string>

namespace todo {

struct IsoWeek {
    int year;
    int week;

    std::string to_string() const;  // YYYY-Www
    bool operator==(const IsoWeek& other) const = default;
};

class Date {
public:
    Date(int year, int month, int day);

    static std::optional<Date> parse(const std::string& text);  // YYYY-MM-DD
    static Date today_local();

    int year() const { return year_; }
    int month() const { return month_; }
    int day() const { return day_; }

    std::string to_string() const;  // YYYY-MM-DD
    std::string compact() const;    // YYYYMMDD
    Date add_days(int days) const;
    IsoWeek iso_week() const;
    int weekday_iso() const;  // Monday = 1 ... Sunday = 7
    std::string weekday_name() const;  // Monday ... Sunday

    auto operator<=>(const Date& other) const = default;

private:
    long long days_since_epoch() const;
    static Date from_days_since_epoch(long long days);

    int year_;
    int month_;
    int day_;
};

// All times are the system's local time; the system time zone is the only one the program uses.
class DateTime {
public:
    static std::string now_local_minutes();  // YYYY-MM-DDTHH:MM
    // The system time zone as it applies now, e.g. "Eastern Standard Time, daylight saving time, UTC-04:00".
    static std::string local_zone_label();
    // The local date (YYYY-MM-DD) of an ISO 8601 time with an offset or Z, e.g. "2024-12-25T16:00:00+00:00";
    // nullopt when the text is not in that form.
    static std::optional<std::string> local_date_of(const std::string& iso_time);
};

}  // namespace todo
