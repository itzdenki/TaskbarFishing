#include "BackgroundDemo.h"
#include "AssetTexture.h"
#include "AssetCatalog.h"
#include "BackgroundClock.h"
#include "platform/LocalTime.h"
#include <raylib.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>

namespace {
constexpr int kWidth = 1024, kHeight = 288;
constexpr float kFadeSeconds = 0.8f;
constexpr Color kInk{230, 241, 236, 255};
constexpr Color kMint{139, 220, 193, 255};
constexpr Color kPanel{12, 24, 33, 220};

struct Window {
    explicit Window(bool hidden) {
        SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_ALWAYS_RUN | (hidden ? FLAG_WINDOW_HIDDEN : 0));
        InitWindow(kWidth, kHeight, "Taskbar Fishing - 24 hour background demo");
        if (!IsWindowReady()) throw std::runtime_error("Could not open background demo");
        SetWindowMinSize(512, 144);
        SetTargetFPS(hidden ? 120 : 60);
    }
    ~Window() { CloseWindow(); }
};

class BackgroundImages {
public:
    ~BackgroundImages() { for (auto texture : textures_) if (texture.id) UnloadTexture(texture); }
    // Cache current, previous and next sheets only (54 MiB of RGBA pixels).
    // The next automatic transition is already loaded before its 15-second boundary.
    void prepare(int hour, int previous) {
        const int next = (hour + 1) % 24;
        for (int i = 0; i < 24; ++i) {
            if (i != hour && i != previous && i != next && textures_[i].id) {
                UnloadTexture(textures_[i]); textures_[i] = {};
            }
        }
        for (int i : {hour, previous, next}) {
            if (i < 0 || textures_[i].id || missing_[i]) continue;
            textures_[i] = loadAssetTexture(AssetCatalog::get().background().hourlyTextures[i]);
            missing_[i] = textures_[i].id == 0;
            if (textures_[i].id) SetTextureFilter(textures_[i], TEXTURE_FILTER_POINT);
        }
    }
    Texture2D at(int hour) const { return textures_[hour]; }
private:
    std::array<Texture2D, 24> textures_{};
    std::array<bool, 24> missing_{};
};

void drawBackground(Texture2D texture, Rectangle viewport, float opacity, double elapsedMs) {
    if (!texture.id) return;
    const auto& atlas = AssetCatalog::get().background();
    DrawTexturePro(texture, atlas.frames.at(atlas.frameAt(elapsedMs)).source,
                   viewport, {0, 0}, 0, Fade(WHITE, opacity));
}

int localHour() {
    return localTimeNow().hour;
}
}

int runBackgroundDemo(bool smokeTest) {
    Window window(smokeTest);
    BackgroundImages images;
    BackgroundClock clock;
    int hour = 0, previous = -1, frame = 0;
    bool live = false, showControls = true;
    float fade = 1;
    std::array<bool, 24> seen{};
    images.prepare(hour, previous);
    double lastTime = GetTime();
    double animationMs = 0;
    TraceLog(LOG_INFO, "BACKGROUND DEMO: 00h, automatic interval 15 seconds");
    while (!WindowShouldClose() && (!smokeTest || frame < 362)) {
        const double now = GetTime();
        const double dt = smokeTest ? (frame == 0 ? 0.0 : 1.0) : now - lastTime;
        lastTime = now;
        animationMs = std::fmod(animationMs + dt * 1000, AssetCatalog::get().background().durationMs);
        if (!live) clock.update(dt);
        if (!smokeTest) {
            if (IsKeyPressed(KEY_T)) { live = !live; if (!live) clock.select(hour); }
            if (IsKeyPressed(KEY_SPACE)) { if (live) { live = false; clock.select(hour); } clock.togglePause(); }
            if (IsKeyPressed(KEY_RIGHT)) { live = false; clock.select(hour + 1); }
            if (IsKeyPressed(KEY_LEFT)) { live = false; clock.select(hour - 1); }
            if (IsKeyPressed(KEY_R)) { live = false; clock = BackgroundClock{}; }
            if (IsKeyPressed(KEY_H)) showControls = !showControls;
        }
        const int selected = live ? localHour() : clock.hour();
        if (selected != hour) {
            previous = hour; hour = selected; fade = 0;
            images.prepare(hour, previous);
            TraceLog(LOG_INFO, "BACKGROUND DEMO: %02dh at %.2f seconds", hour, now);
        } else fade = std::min(1.0f, fade + static_cast<float>(dt) / kFadeSeconds);
        if (smokeTest && !images.at(hour).id) throw std::runtime_error("Missing hourly background " + std::to_string(hour));
        seen[hour] = images.at(hour).id != 0;

        const float scale = std::min(GetScreenWidth() / static_cast<float>(kWidth), GetScreenHeight() / static_cast<float>(kHeight));
        const Rectangle viewport{(GetScreenWidth() - kWidth * scale) / 2, (GetScreenHeight() - kHeight * scale) / 2,
                                 kWidth * scale, kHeight * scale};
        BeginDrawing();
        ClearBackground({8, 16, 23, 255});
        if (previous >= 0 && fade < 1) drawBackground(images.at(previous), viewport, 1, animationMs);
        drawBackground(images.at(hour), viewport, fade, animationMs);
        if (!images.at(hour).id)
            DrawText(TextFormat("Missing assets/background/hour_%02d.png", hour), 30, GetScreenHeight() / 2, 20, ORANGE);
        if (showControls) {
            const int x = static_cast<int>(viewport.x + 16), y = static_cast<int>(viewport.y + 16);
            DrawRectangleRounded({static_cast<float>(x), static_cast<float>(y), 254, 78}, 0.16f, 6, kPanel);
            DrawText(TextFormat("%02d:00", hour), x + 14, y + 10, 28, kInk);
            DrawText(live ? "LOCAL TIME" : clock.paused() ? "PAUSED" : "15s / HOUR", x + 114, y + 18, 16, kMint);
            DrawText(live ? "Background follows your local hour" : clock.paused() ? "SPACE to continue" :
                TextFormat("Next: %02d:00  in %02ds", (hour + 1) % 24, static_cast<int>(std::ceil(clock.remaining()))),
                x + 14, y + 50, 13, kInk);
            const int bottom = static_cast<int>(viewport.y + viewport.height - 48);
            DrawRectangle(static_cast<int>(viewport.x), bottom, static_cast<int>(viewport.width), 48, kPanel);
            DrawText("SPACE pause   LEFT/RIGHT hour   T clock   R restart   H hide   ESC close",
                     static_cast<int>(viewport.x + 16), bottom + 19, GetScreenWidth() < 850 ? 10 : 14, kInk);
            if (!live) {
                DrawRectangle(static_cast<int>(viewport.x), bottom, static_cast<int>(viewport.width), 3, {41, 64, 71, 255});
                DrawRectangle(static_cast<int>(viewport.x), bottom, static_cast<int>(viewport.width * clock.progress()), 3, kMint);
            }
        }
        EndDrawing();
        // Two stable frames before reading a hidden double-buffered window.
        if (smokeTest && frame % 90 == 2) {
            const auto directory = assets::applicationDirectory(GetApplicationDirectory()) / "captures/background";
            std::filesystem::create_directories(directory);
            const auto path = directory / TextFormat("%02dh.png", hour);
            const auto screen = LoadImageFromScreen();
            int byteCount = 0;
            std::unique_ptr<unsigned char, decltype(&MemFree)> png(ExportImageToMemory(screen, ".png", &byteCount), MemFree);
            UnloadImage(screen);
            std::ofstream output(path, std::ios::binary);
            if (png && byteCount > 0) output.write(reinterpret_cast<const char*>(png.get()), byteCount);
            output.close();
            if (!png || byteCount <= 0 || !output) throw std::runtime_error("Could not capture background demo");
        }
        ++frame;
    }
    if (smokeTest) {
        if (frame != 362 || !std::all_of(seen.begin(), seen.end(), [](bool loaded) { return loaded; }) || hour != 0)
            throw std::runtime_error("Background demo did not complete all 24 hours and midnight wrap");
        TraceLog(LOG_INFO, "BACKGROUND SMOKE PASS: 24 images rendered, 23h -> 00h verified");
    }
    return 0;
}
