#pragma once
#include <string>

struct LocalTimeSnapshot {
    int hour = 12;
    int minute = 0;
    int utcOffsetMinutes = 0;
    std::string label() const;
};

// Re-reads Windows time zone settings, including daylight saving, on each call.
LocalTimeSnapshot localTimeNow();
