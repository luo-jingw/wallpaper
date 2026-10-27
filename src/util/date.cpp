#include "util/date.h"

#include <windows.h>

#include <cstdio>
#include <cstdlib>
#include <initializer_list>
#include <optional>
#include <string>

#include "util/utf8.h"

namespace todo {

namespace {

bool is_leap(int year) {
    return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
}

int days_in_month(int year, int month) {
    static const int kDays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    return month == 2 && is_leap(year) ? 29 : kDays[month - 1];
}

}  // namespace

std::string IsoWeek::to_string() const {
    char buffer[16];
    std::snprintf(buffer, sizeof(buffer), "%04d-W%02d", year, week);
    return buffer;
}

Date::Date(int year, int month, int day) : year_(year), month_(month), day_(day) {}

std::optional<Date> Date::parse(const std::string& text) {
    if (text.size() != 10 || text[4] != '-' || text[7] != '-') {
        return std::nullopt;
    }
    for (const int index : {0, 1, 2, 3, 5, 6, 8, 9}) {
        const char c = text[static_cast<size_t>(index)];
        if (c < '0' || c > '9') {
            return std::nullopt;
        }
    }
    const int year = std::stoi(text.substr(0, 4));
    const int month = std::stoi(text.substr(5, 2));
    const int day = std::stoi(text.substr(8, 2));
    if (month < 1 || month > 12 || day < 1 || day > days_in_month(year, month)) {
        return std::nullopt;
    }
    return Date(year, month, day);
}

Date Date::today_local() {
    SYSTEMTIME now;
    GetLocalTime(&now);
    return Date(now.wYear, now.wMonth, now.wDay);
}

std::string Date::to_string() const {
    char buffer[16];
    std::snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d", year_, month_, day_);
    return buffer;
}

std::string Date::compact() const {
    char buffer[16];
    std::snprintf(buffer, sizeof(buffer), "%04d%02d%02d", year_, month_, day_);
    return buffer;
}

// Howard Hinnant's days_from_civil / civil_from_days.
long long Date::days_since_epoch() const {
    const int y = month_ <= 2 ? year_ - 1 : year_;
    const long long era = (y >= 0 ? y : y - 399) / 400;
    const unsigned yoe = static_cast<unsigned>(y - era * 400);
    const unsigned mp = static_cast<unsigned>(month_ + (month_ > 2 ? -3 : 9));
    const unsigned doy = (153 * mp + 2) / 5 + static_cast<unsigned>(day_) - 1;
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + static_cast<long long>(doe) - 719468;
}

Date Date::from_days_since_epoch(long long days) {
    days += 719468;
    const long long era = (days >= 0 ? days : days - 146096) / 146097;
    const unsigned doe = static_cast<unsigned>(days - era * 146097);
    const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    const long long y = static_cast<long long>(yoe) + era * 400;
    const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const unsigned mp = (5 * doy + 2) / 153;
    const unsigned d = doy - (153 * mp + 2) / 5 + 1;
    const unsigned m = mp < 10 ? mp + 3 : mp - 9;
    return Date(static_cast<int>(m <= 2 ? y + 1 : y), static_cast<int>(m), static_cast<int>(d));
}

Date Date::add_days(int days) const {
    return from_days_since_epoch(days_since_epoch() + days);
}

int Date::weekday_iso() const {
    // 1970-01-01 was a Thursday (ISO weekday 4).
    const long long days = days_since_epoch();
    const long long index = ((days % 7) + 7 + 3) % 7;  // Monday = 0
    return static_cast<int>(index) + 1;
}

IsoWeek Date::iso_week() const {
    const Date thursday = add_days(4 - weekday_iso());
    const Date jan1(thursday.year(), 1, 1);
    const long long ordinal = thursday.days_since_epoch() - jan1.days_since_epoch();
    return IsoWeek{thursday.year(), static_cast<int>(ordinal / 7) + 1};
}

std::string Date::weekday_name() const {
    static const char* const kNames[7] = {"Monday", "Tuesday", "Wednesday", "Thursday",
                                          "Friday", "Saturday", "Sunday"};
    return kNames[weekday_iso() - 1];
}

std::string DateTime::now_local_minutes() {
    SYSTEMTIME now;
    GetLocalTime(&now);
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%04d-%02d-%02dT%02d:%02d", now.wYear, now.wMonth, now.wDay, now.wHour,
                  now.wMinute);
    return buffer;
}

std::string DateTime::local_zone_label() {
    DYNAMIC_TIME_ZONE_INFORMATION zone{};
    const DWORD kind = GetDynamicTimeZoneInformation(&zone);
    const bool daylight = kind == TIME_ZONE_ID_DAYLIGHT;
    // Bias is minutes to add to local time to get UTC.
    const long bias = zone.Bias + (daylight ? zone.DaylightBias : kind == TIME_ZONE_ID_STANDARD ? zone.StandardBias : 0);
    const long offset = -bias;
    char buffer[16];
    std::snprintf(buffer, sizeof(buffer), "UTC%c%02ld:%02ld", offset < 0 ? '-' : '+', std::labs(offset) / 60,
                  std::labs(offset) % 60);
    // The Windows zone id is English whatever the display language ("Eastern Standard Time" also in summer).
    const std::wstring id = zone.TimeZoneKeyName;
    return (id.empty() ? std::string() : Utf8::from_wide(id) + ", ") + (daylight ? "daylight saving time, " : "") +
           buffer;
}

std::optional<std::string> DateTime::local_date_of(const std::string& iso_time) {
    int year = 0, month = 0, day = 0, hour = 0, minute = 0, second = 0;
    char sign = 0;
    int offset_hours = 0, offset_minutes = 0;
    if (sscanf_s(iso_time.c_str(), "%4d-%2d-%2dT%2d:%2d:%2d", &year, &month, &day, &hour, &minute, &second) != 6) {
        return std::nullopt;
    }
    const size_t zone_at = iso_time.find_first_of("Z+-", 19);
    if (zone_at == std::string::npos) {
        return std::nullopt;
    }
    if (iso_time[zone_at] != 'Z') {
        sign = iso_time[zone_at];
        if (sscanf_s(iso_time.c_str() + zone_at + 1, "%2d:%2d", &offset_hours, &offset_minutes) != 2) {
            return std::nullopt;
        }
    }
    SYSTEMTIME given{};
    given.wYear = static_cast<WORD>(year);
    given.wMonth = static_cast<WORD>(month);
    given.wDay = static_cast<WORD>(day);
    given.wHour = static_cast<WORD>(hour);
    given.wMinute = static_cast<WORD>(minute);
    given.wSecond = static_cast<WORD>(second);
    FILETIME file{};
    if (!SystemTimeToFileTime(&given, &file)) {
        return std::nullopt;
    }
    // To UTC: subtract the stated offset (100 ns units).
    ULARGE_INTEGER ticks{};
    ticks.LowPart = file.dwLowDateTime;
    ticks.HighPart = file.dwHighDateTime;
    const long long offset_ticks = (offset_hours * 60LL + offset_minutes) * 60LL * 10000000LL;
    ticks.QuadPart = sign == '+' ? ticks.QuadPart - offset_ticks : sign == '-' ? ticks.QuadPart + offset_ticks
                                                                              : ticks.QuadPart;
    file.dwLowDateTime = ticks.LowPart;
    file.dwHighDateTime = ticks.HighPart;
    SYSTEMTIME utc{};
    SYSTEMTIME local{};
    if (!FileTimeToSystemTime(&file, &utc) || !SystemTimeToTzSpecificLocalTime(nullptr, &utc, &local)) {
        return std::nullopt;
    }
    char buffer[16];
    std::snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d", local.wYear, local.wMonth, local.wDay);
    return std::string(buffer);
}

}  // namespace todo
