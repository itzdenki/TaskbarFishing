#pragma once
#include <cmath>

// Demo time is independent of fishing speed and save data.
class BackgroundClock {
public:
    static constexpr double kSecondsPerHour = 15.0;
    void update(double seconds) {
        if (paused_ || !std::isfinite(seconds) || seconds <= 0) return;
        const double elapsed = elapsed_ + std::fmod(seconds, 24 * kSecondsPerHour);
        hour_ = (hour_ + static_cast<int>(elapsed / kSecondsPerHour)) % 24;
        elapsed_ = std::fmod(elapsed, kSecondsPerHour);
    }
    void select(int hour) { hour_ = (hour % 24 + 24) % 24; elapsed_ = 0; }
    void togglePause() { paused_ = !paused_; }
    bool paused() const { return paused_; }
    int hour() const { return hour_; }
    double remaining() const { return kSecondsPerHour - elapsed_; }
    float progress() const { return static_cast<float>(elapsed_ / kSecondsPerHour); }
private:
    int hour_ = 0;
    double elapsed_ = 0;
    bool paused_ = false;
};
