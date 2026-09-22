#pragma once
#include <optional>
#include <raylib.h>

class WindowDrag {
public:
    std::optional<Vector2> update(Vector2 cursor, Vector2 window, bool pressed, bool held, bool draggable) {
        if (pressed && held && draggable) {
            offset_={cursor.x-window.x,cursor.y-window.y};
            active_=true;
        }
        if (!held) active_=false;
        if (!active_) return std::nullopt;
        return Vector2{cursor.x-offset_.x,cursor.y-offset_.y};
    }
    void cancel() { active_=false; }
    bool active() const { return active_; }
private:
    Vector2 offset_{};
    bool active_=false;
};
