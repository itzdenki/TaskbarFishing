#pragma once
#include <array>

struct HourlyConditions { int successPercent; const char* level; };
inline constexpr std::array<HourlyConditions, 24> kHourlyConditions{{
    {42, "Medium"}, {38, "Medium"}, {34, "Low"}, {32, "Low"},
    {40, "Medium"}, {58, "High"}, {78, "Very high"}, {85, "Peak"},
    {72, "Very high"}, {60, "High"}, {48, "Medium"}, {38, "Low"},
    {30, "Very low"}, {28, "Lowest"}, {32, "Low"}, {42, "Medium"},
    {58, "High"}, {72, "Very high"}, {88, "Peak"}, {82, "Very high"},
    {68, "High"}, {55, "High"}, {48, "Medium"}, {44, "Medium"}
}};
inline constexpr HourlyConditions conditionsForHour(int hour) {
    return kHourlyConditions[static_cast<unsigned>(hour % 24 + 24) % 24];
}
