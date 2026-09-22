#pragma once
#include <array>
#include <raylib.h>

class HourlyBackground {
public:
    ~HourlyBackground() { release(); }
    HourlyBackground() = default;
    HourlyBackground(const HourlyBackground&) = delete;
    HourlyBackground& operator=(const HourlyBackground&) = delete;
    void release();
    void update(int hour, float dt, bool animate = true);
    bool draw(Rectangle destination) const;
    int hour() const { return hour_; }
    bool available() const { return hour_ >= 0 && textures_[hour_].id != 0; }
    int frameIndex() const;
private:
    std::array<Texture2D, 24> textures_{};
    std::array<bool, 24> missing_{};
    int hour_ = -1, previous_ = -1;
    float fade_ = 1;
    double elapsedMs_ = 0;
};
