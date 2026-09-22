#pragma once
#include "fishing/FishingSystem.h"
#include "player/Player.h"
#include "platform/LocalTime.h"
#include "HourlyBackground.h"
#include "UISkin.h"
#include "FishingSprites.h"
#include "platform/GamePreferences.h"
#include <cmath>
#include <optional>
#include <string>
#include <raylib.h>

enum class UIActionType { None, SellStored, SellAll, ToggleLock, UpgradeRod, UpgradeBox, UpgradeStat, ToggleAutoSell, ToggleTopmost, Dock, Minimize, Close, SetPreference, SaveNow, ResetPreferences, ResetPosition, ResetScale, OpenSaveFolder, FinishOnboarding };
struct UIAction { UIActionType type = UIActionType::None; std::size_t index = 0; int value = 0; };
struct UIInput {
    Vector2 mouse{-1, -1};
    bool pressed = false;      // left button pressed this frame
    bool released = false;     // left button released this frame
    bool rightPressed = false; // right button pressed this frame (quick sell in the Fish Box)
    bool altDown = false;      // Alt held: left click locks/unlocks instead of selecting
    bool down = false;
    float wheel = 0;
};

class UI {
public:
    UI() = default;
    ~UI();
    UI(const UI&) = delete;
    UI& operator=(const UI&) = delete;
    void releaseGraphics();
    // Fish Box grid geometry (shared with tests). Slots are laid out row-major, kGridCols per row.
    static constexpr int kGridCols = 10;
    static constexpr int kVisibleRows = 3;
    static constexpr int kGridSlots = 70;
    static constexpr int kWidth = 512;
    static constexpr int kSceneHeight = 144; // Native 32:9 animated atlas, plus the last-catch strip.
    static constexpr int kCompactHeight = kSceneHeight + 32;
    static constexpr int kExpandedHeight = kCompactHeight + 252;
    static constexpr int kSettingsHeight = kCompactHeight + 372;
    static constexpr int kDashboardHeight = kCompactHeight + 360;
    static constexpr int kStatsWidth = 960, kStatsHeight = 640;
    static constexpr int kSettingsTabs = 5;
    static Rectangle slotRect(int slot, int scrollRows = 0);
    int inventoryScroll() const { return inventoryScroll_; }

    UIAction draw(const FishingSystem& fishing, const Player& player, const std::optional<FishInstance>& lastCatch, bool autoSellCommon, bool topmost, UIInput input);
    void notifyCatch(CatchMilestone milestone) { milestone_ = milestone; noticeSeconds_ = 4.0f; }
    void update(float dt);
    void setLocalTime(LocalTimeSnapshot time) { localTime_ = time; }
    void setPreferences(GamePreferences value) { preferences_=value; }
    void setSavePath(std::string path) { savePath_ = std::move(path); }
    const GamePreferences& preferences() const { return preferences_; }
    // Layout stays in logical 512-wide units; the window is scaled by preferences().scale.
    float scale() const { return preferences_.scale / 100.0f; }
    int windowWidth() const { return scaled(width()); }
    int windowHeight() const { return scaled(height()); }
    int scaled(int logical) const { return static_cast<int>(std::lround(logical * scale())); }
    Vector2 toLogical(Vector2 window) const { return {window.x / scale(), window.y / scale()}; }
    // Camera that maps logical coordinates to window pixels; offset is in logical units.
    Camera2D camera(Vector2 offset = {0, 0}) const { const float s = scale(); return {{offset.x * s, offset.y * s}, {0, 0}, 0, s}; }
    bool canDrag(Vector2 mouse) const;
    int backgroundHour() const { return background_.hour(); }
    bool backgroundAvailable() const { return background_.available(); }
    int backgroundFrame() const { return background_.frameIndex(); }
    const std::string& playerAnimation() const { return sprites_.animation(); }
    int playerFrame() const { return sprites_.playerFrame(); }
    void toggleBox() { if (onboardingOpen_) return; closePanels(); extrasOpen_ = boxOpen_ = true; collectionOpen_ = false; }
    void toggleCollection() { if (onboardingOpen_) return; closePanels(); extrasOpen_ = boxOpen_ = collectionOpen_ = true; }
    void toggleExtras() { if (onboardingOpen_) return; if (extrasOpen_) closePanels(); else { closePanels(); extrasOpen_ = boxOpen_ = true; } }
    void toggleSettings() { if (onboardingOpen_) return; const bool open = !settingsOpen_; closePanels(); settingsOpen_ = open; }
    void closePanels() { boxOpen_ = collectionOpen_ = extrasOpen_ = settingsOpen_ = craftOpen_ = summaryOpen_ = false; inventoryScroll_ = 0; pressedButton_.reset(); }
    void toggleStats() { if (onboardingOpen_) return; statsOpen_ = !statsOpen_; pressedButton_.reset(); treePress_.reset(); }
    void beginOnboarding();
    void endOnboarding();
    void onboardingBack();
    bool setOnboardingName(std::string value);
    bool onboardingOpen() const { return onboardingOpen_; }
    int onboardingStep() const { return onboardingStep_; }
    const std::string& onboardingName() const { return onboardingName_; }
    void backFromStats() { statsOpen_ = false; pressedButton_.reset(); treePress_.reset(); }
    bool statsOpen() const { return statsOpen_; }
    bool dashboardOpen() const { return extrasOpen_ && boxOpen_ && !statsOpen_; }
    int dashboardX() const { return dashboardOpen() && craftOpen_ ? 296 : 0; }
    int width() const { return statsOpen_ ? kStatsWidth : kWidth + (dashboardOpen() ? 296 * (static_cast<int>(craftOpen_) + static_cast<int>(summaryOpen_)) : 0); }
    Rectangle statsNodeRect(UpgradeId id) const;
    UpgradeId selectedUpgrade() const { return selectedUpgrade_; }
    float statsZoom() const { return treeZoom_; }
    bool boxOpen() const { return boxOpen_ && !statsOpen_; }
    bool extrasOpen() const { return extrasOpen_; }
    bool settingsOpen() const { return settingsOpen_; }
    bool collectionTab() const { return collectionOpen_; }
    int height() const { return statsOpen_ ? kStatsHeight : onboardingOpen_ ? kSettingsHeight : dashboardOpen() ? kDashboardHeight : settingsOpen_ ? kSettingsHeight : extrasOpen_ ? kExpandedHeight : kCompactHeight; }
    int settingsTab() const { return settingsTab_; }
    Rectangle dragRect() const { return statsOpen_ ? Rectangle{16, 10, 620, 40} : Rectangle{static_cast<float>(dashboardX()), static_cast<float>(height() - kCompactHeight + 32), 446, kSceneHeight - 32}; }
    std::optional<std::size_t> selectedSlot() const { return selected_; }
private:
    bool clicked(Rectangle rect);
    bool button(Rectangle rect, const std::string& label, bool enabled = true, bool selected = false, int icon = -1, bool tab = false);
    // Draggable slider; reports a new stepped value while pressed or when scrolled over.
    std::optional<int> slider(Rectangle track, int value, int min, int max, int step);
    void card(Rectangle rect, Color fill = {23,39,49,255}, Color border = {48,70,79,255}, float radius = 0) const;
    void track(Rectangle rect, float progress, Color accent) const;
    void initFont();
    void text(const char* label, float x, float y, float size, Color color) const;
    float textWidth(const char* label, float size) const;
    void fitText(const std::string& label, Rectangle rect, float size, Color color) const;
    void scene(const FishingSystem& fishing, const Player& player, bool autoSellCommon);
    void catchPanel(const FishingSystem& fishing, const Player& player, bool autoSellCommon);
    void collectionPanel(const FishingSystem& fishing, const Player& player);
    UIAction fishBoxPanel(const FishingSystem& fishing, const Player& player);
    UIAction statsPanel(const FishingSystem& fishing, const Player& player);
    UIAction dashboard(const FishingSystem& fishing, const Player& player, const std::optional<FishInstance>& lastCatch, bool autoSellCommon);
    UIAction equipmentPanel(const Player& player, float x);
    UIAction summaryPanel(const FishingSystem& fishing, const Player& player, float x);
    UIAction settingsPanel(bool autoSellCommon, bool topmost);
    UIAction welcomePanel(bool autoSellCommon, bool topmost);
    void consumeNameInput();
    void fishIcon(const FishInstance& fish, const FishSpecies& species, Rectangle cell);
    void itemTooltip(const FishInstance& fish, const FishSpecies& species, const Player& player) const;
    bool boxOpen_ = false;
    bool collectionOpen_ = false;
    bool extrasOpen_ = false;
    bool settingsOpen_ = false;
    bool statsOpen_ = false;
    bool craftOpen_ = false, summaryOpen_ = false;
    UpgradeId craftSelection_ = UpgradeId::Rod2;
    Texture2D portrait_{};
    UpgradeId selectedUpgrade_ = UpgradeId::Rod2;
    float treeZoom_ = 1;
    Vector2 treePan_{};
    std::optional<Vector2> treePress_;
    Vector2 treePanAtPress_{};
    bool treeDragging_ = false;
    CatchMilestone milestone_;
    std::optional<std::size_t> selected_;
    std::optional<std::size_t> hovered_;
    int collectionScroll_ = 0;
    int inventoryScroll_ = 0;
    UIInput input_;
    std::optional<Rectangle> pressedButton_;
    float noticeSeconds_ = 0;
    std::string tooltip_;
    Font font_{};
    bool fontReady_ = false;
    bool ownsFont_ = false;
    LocalTimeSnapshot localTime_{};
    HourlyBackground background_;
    UISkin skin_;
    FishingSprites sprites_;
    float frameDt_ = 0;
    GamePreferences preferences_;
    int settingsTab_ = 0;
    std::string savePath_;
    bool onboardingOpen_ = false;
    int onboardingStep_ = 0;
    bool nameFocused_ = true;
    std::string onboardingName_;
};
