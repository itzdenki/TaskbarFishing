#pragma once
#include "fishing/FishingSystem.h"
#include "player/Player.h"
#include "ui/UI.h"
#include "save/SaveSystem.h"
#include "platform/WindowDrag.h"

class Game {
public:
    explicit Game(bool smoke, std::filesystem::path savePath = {}, bool discordEnabled = true);
    ~Game();
    Game(const Game&) = delete;
    Game& operator=(const Game&) = delete;
    void run(bool showStats = false);
private:
    void save();
    bool handleAction(UIAction action);
    // Moves a settled catch into the Fish Box (or auto-sells it). Returns true when fishing resumed.
    bool settleCatch();
    void handleWindowDrag();
    void syncWindowSize();
    UIAction keyboardInput();
    // Pushes preference side effects (vsync, autostart, UI) after preferences_ changed from `previous`.
    void applyPreferences(const GamePreferences& previous);
    // Eases the window toward the active or idle opacity.
    void updateOpacity(float dt, bool active);
    void dock(bool primaryMonitor = false);
    bool smoke_;
    bool discordEnabled_;
    FishingSystem fishing_;
    std::optional<FishInstance> lastCatch_;
    Player player_;
    UI ui_;
    std::filesystem::path savePath_;
    bool savingEnabled_ = true;
    bool autoSellCommon_ = false;
    bool alwaysOnTop_ = false;
    std::string saveWarning_;
    WindowDrag drag_;
    bool statsWindow_ = false;
    Vector2 widgetPosition_{};
    int dashboardOffset_ = 0; // window pixels
    GamePreferences preferences_;
    bool preferencesDirty_ = false; // Slider drags change preferences every frame; save once the button is released.
    float opacity_ = 1, appliedOpacity_ = 1;
};
