#include "LocalTime.h"
#include <cstdio>
#include <cstdlib>
#include <ctime>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

std::string LocalTimeSnapshot::label() const {
    char buffer[40];
    const int offset = std::abs(utcOffsetMinutes);
    std::snprintf(buffer, sizeof(buffer), "%02d:%02d  UTC%c%02d:%02d", hour, minute,
                  utcOffsetMinutes < 0 ? '-' : '+', offset / 60, offset % 60);
    return buffer;
}

LocalTimeSnapshot localTimeNow() {
#ifdef _WIN32
    SYSTEMTIME utc{}, local{};
    GetSystemTime(&utc);
    DYNAMIC_TIME_ZONE_INFORMATION zone{};
    const auto status = GetDynamicTimeZoneInformation(&zone);
    if (status != TIME_ZONE_ID_INVALID && SystemTimeToTzSpecificLocalTimeEx(&zone, &utc, &local)) {
        FILETIME utcFile{}, localFile{};
        SystemTimeToFileTime(&utc, &utcFile);
        SystemTimeToFileTime(&local, &localFile);
        const auto ticks = [](FILETIME value) {
            return (static_cast<long long>(value.dwHighDateTime) << 32) | value.dwLowDateTime;
        };
        const int offset = static_cast<int>((ticks(localFile) - ticks(utcFile)) / 600000000LL);
        return {local.wHour, local.wMinute, offset};
    }
    GetLocalTime(&local);
    return {local.wHour, local.wMinute, 0};
#else
    const auto now = std::time(nullptr);
    std::tm local{};
    localtime_r(&now, &local);
    char zone[16]{};
    std::strftime(zone, sizeof(zone), "%z", &local);
    const int value = std::atoi(zone);
    return {local.tm_hour, local.tm_min, value / 100 * 60 + value % 100};
#endif
}
