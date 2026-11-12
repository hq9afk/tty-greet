#pragma once

#include <array>
#include <ctime>

/*
Date and time in local time (or UTC), e.g. `Thu 2026-11-12 14:03:27`, NUL-terminated.
The weekday follows the current locale.
*/
inline std::array<char, 64> format_date(std::time_t t, bool utc = false) {
    std::tm tm{};
    utc ? gmtime_r(&t, &tm) : localtime_r(&t, &tm);
    std::array<char, 64> out{};
    strftime(out.data(), out.size(), "%a %Y-%m-%d %H:%M:%S", &tm);
    return out;
}
