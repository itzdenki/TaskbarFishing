#include "ui/BackgroundClock.h"
#include "platform/LocalTime.h"
#include <iostream>
#include <limits>
#include <stdexcept>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

void check(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
int main() {
    try {
        BackgroundClock clock;
        clock.update(14.5);
        check(clock.hour() == 0 && clock.remaining() == 0.5, "hold background for 15 seconds");
        clock.update(0.5);
        check(clock.hour() == 1 && clock.remaining() == 15, "advance at exactly 15 seconds");
        clock.togglePause(); clock.update(60);
        check(clock.hour() == 1 && clock.remaining() == 15, "pause freezes countdown");
        clock.togglePause(); clock.select(23); clock.update(15.25);
        check(clock.hour() == 0 && clock.remaining() == 14.75, "midnight wrap preserves excess time");
        clock.update(360 * 5 + 30);
        check(clock.hour() == 2 && clock.remaining() == 14.75, "long stalls advance modulo one day");
        clock.select(-1); check(clock.hour() == 23 && clock.remaining() == 15, "backward manual wrap resets timer");
        clock.select(24); check(clock.hour() == 0, "forward manual wrap");
        clock.update(-1); clock.update(std::numeric_limits<double>::quiet_NaN());
        check(clock.hour() == 0 && clock.remaining() == 15, "invalid deltas ignored");
        check(LocalTimeSnapshot{5, 45, 345}.label() == "05:45  UTC+05:45", "fractional positive timezone label");
        check(LocalTimeSnapshot{23, 30, -210}.label() == "23:30  UTC-03:30", "fractional negative timezone label");
#ifdef _WIN32
        SYSTEMTIME before{}, after{};
        GetLocalTime(&before);
        const auto local = localTimeNow();
        GetLocalTime(&after);
        check((local.hour == before.wHour && local.minute == before.wMinute) ||
              (local.hour == after.wHour && local.minute == after.wMinute), "production clock matches Windows local time");
        TIME_ZONE_INFORMATION zone{};
        const auto zoneState = GetTimeZoneInformation(&zone);
        check(zoneState != TIME_ZONE_ID_INVALID, "read active Windows timezone");
        const int expectedOffset = -zone.Bias - (zoneState == TIME_ZONE_ID_DAYLIGHT ? zone.DaylightBias :
            zoneState == TIME_ZONE_ID_STANDARD ? zone.StandardBias : 0);
        check(local.utcOffsetMinutes == expectedOffset, "production UTC offset matches Windows daylight-saving state");
        std::cout << "Windows clock: " << local.label() << '\n';
#endif
        std::cout << "Background clock tests passed\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
