#include "Game.h"
#include "platform/DesktopWindow.h"
#include "platform/DiscordPresence.h"
#include "platform/EmbeddedAssets.h"
#include <raylib.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <stdexcept>

namespace {
constexpr char kDiscordApplicationId[] = "1547528683689087106";
}

Game::Game(bool smoke, std::filesystem::path savePath, bool discordEnabled) : smoke_(smoke), discordEnabled_(discordEnabled && !smoke), fishing_(smoke ? 42u : std::random_device{}()), savePath_(savePath.empty() ? SaveSystem::defaultPath() : std::move(savePath)) {
    if (!smoke_) {
        SaveData data;
        const auto status = SaveSystem::load(savePath_, data, fishing_.database(), saveWarning_);
        savingEnabled_ = status != LoadStatus::Invalid;
        if (status == LoadStatus::Loaded || status == LoadStatus::RecoveredBackup) {
            player_ = std::move(data.player);
            autoSellCommon_ = data.autoSellCommon;
            alwaysOnTop_ = data.alwaysOnTop;
            preferences_ = data.preferences;
            if (data.pendingCatch) { fishing_.restoreCatch(*data.pendingCatch); lastCatch_ = data.pendingCatch; }
        } else if (status == LoadStatus::Missing) {
            ui_.beginOnboarding();
        }
        if (status == LoadStatus::RecoveredBackup) saveWarning_ = "Recovered previous save from backup.";
    }
    ui_.setPreferences(preferences_);
    ui_.setSavePath(savePath_.string());
    SetConfigFlags(FLAG_WINDOW_UNDECORATED | FLAG_WINDOW_ALWAYS_RUN | FLAG_WINDOW_UNFOCUSED |
                   (preferences_.vsync ? FLAG_VSYNC_HINT : 0) | (smoke ? FLAG_WINDOW_HIDDEN : 0));
    InitWindow(ui_.windowWidth(), ui_.windowHeight(), "Taskbar Fishing");
    if (!IsWindowReady()) throw std::runtime_error("Could not create game window");
    SetTargetFPS(preferences_.fps);
    SetExitKey(KEY_NULL);
    dock();
    if (preferences_.rememberPosition && preferences_.hasPosition) {
        SetWindowPosition(preferences_.windowX,preferences_.windowY);
        desktop::keepInWorkArea(GetWindowHandle());
    }
    opacity_ = appliedOpacity_ = preferences_.opacity / 100.0f;
    SetWindowOpacity(opacity_);
    // Refresh the autostart command in case the executable moved; never touch the registry otherwise.
    if (!smoke_ && preferences_.startWithWindows) desktop::setStartWithWindows(true);
    if (alwaysOnTop_) SetWindowState(FLAG_WINDOW_TOPMOST);
}

void Game::dock(bool primaryMonitor) {
    desktop::dockAboveTaskbar(GetWindowHandle(), {preferences_.dockAlignment, preferences_.taskbarGap, primaryMonitor});
}

void Game::applyPreferences(const GamePreferences& previous) {
    if (preferences_.vsync != previous.vsync) {
        if (preferences_.vsync) SetWindowState(FLAG_VSYNC_HINT); else ClearWindowState(FLAG_VSYNC_HINT);
    }
    if (!smoke_ && !ui_.onboardingOpen() && preferences_.startWithWindows != previous.startWithWindows) {
        if (!desktop::setStartWithWindows(preferences_.startWithWindows)) {
            preferences_.startWithWindows = previous.startWithWindows;
            saveWarning_ = "Could not update the Windows startup entry.";
        }
    }
    if (preferences_.dockAlignment != previous.dockAlignment || preferences_.taskbarGap != previous.taskbarGap) dock();
    ui_.setPreferences(preferences_);
}

void Game::updateOpacity(float dt, bool active) {
    const int idle = std::min(preferences_.opacity, preferences_.idleOpacity);
    const float target = (active ? preferences_.opacity : idle) / 100.0f;
    opacity_ += (target - opacity_) * std::clamp(dt * 8.0f, 0.0f, 1.0f);
    if (std::fabs(target - opacity_) < 0.005f) opacity_ = target;
    if (std::fabs(opacity_ - appliedOpacity_) >= 0.002f) {
        SetWindowOpacity(opacity_);
        appliedOpacity_ = opacity_;
    }
}

Game::~Game() { ui_.releaseGraphics(); CloseWindow(); }

void Game::save() {
    if (smoke_ || !savingEnabled_ || ui_.onboardingOpen()) return;
    if (preferences_.rememberPosition && !ui_.statsOpen()) {
        // Store the compact lake's top-left in window pixels at the current scale.
        const auto position=GetWindowPosition();
        preferences_.windowX=static_cast<int>(position.x)+ui_.scaled(ui_.dashboardX());
        preferences_.windowY=static_cast<int>(position.y)+ui_.scaled(ui_.height()-UI::kCompactHeight);
        preferences_.hasPosition=true;
    }
    SaveData data{player_, fishing_.state() == FishingState::Caught ? fishing_.catchResult() : std::nullopt, autoSellCommon_, alwaysOnTop_, preferences_};
    std::string error;
    if (!SaveSystem::save(savePath_, data, fishing_.database(), error)) {
        saveWarning_ = "SAVE FAILED - check disk space / folder access.";
        TraceLog(LOG_WARNING, "%s", error.c_str());
    } else saveWarning_.clear();
}

void Game::handleWindowDrag() {
    if (smoke_) return;
    if (preferences_.lockWindow) { drag_.cancel(); SetMouseCursor(MOUSE_CURSOR_DEFAULT); return; }
    const auto windowMouse = GetMousePosition();
    const auto mouse = ui_.toLogical(windowMouse);
    SetMouseCursor(drag_.active() || ui_.canDrag(mouse) ? MOUSE_CURSOR_RESIZE_ALL : MOUSE_CURSOR_DEFAULT);
    int screenX = 0, screenY = 0;
    if (!desktop::cursorPosition(screenX, screenY)) {
        const auto origin = GetWindowPosition();
        screenX = static_cast<int>(origin.x + windowMouse.x);
        screenY = static_cast<int>(origin.y + windowMouse.y);
    }
    bool held=IsMouseButtonDown(MOUSE_BUTTON_LEFT);
#ifdef _WIN32
    held=desktop::leftButtonDown();
#endif
    const bool wasDragging=drag_.active();
    const auto position=drag_.update({static_cast<float>(screenX),static_cast<float>(screenY)},GetWindowPosition(),
                                    IsMouseButtonPressed(MOUSE_BUTTON_LEFT),held,ui_.canDrag(mouse));
    if (position) SetWindowPosition(static_cast<int>(position->x),static_cast<int>(position->y));
    if (wasDragging && !drag_.active()) {
        desktop::keepInWorkArea(GetWindowHandle());
        save();
    }
}

void Game::syncWindowSize() {
    const int oldHeight = GetScreenHeight();
    const int oldWidth = GetScreenWidth();
    const int newWidth = ui_.windowWidth(), newHeight = ui_.windowHeight();
    if (oldHeight == newHeight && oldWidth == newWidth) return;
    const auto origin = GetWindowPosition();
    if (ui_.statsOpen() && !statsWindow_) widgetPosition_ = origin;
    SetWindowSize(newWidth, newHeight);
    // Expand upward, keeping the fishing scene in place above the taskbar.
    const int dashboardPixels = ui_.scaled(ui_.dashboardX());
    if (statsWindow_ && !ui_.statsOpen()) SetWindowPosition(static_cast<int>(widgetPosition_.x),static_cast<int>(widgetPosition_.y));
    else SetWindowPosition(static_cast<int>(origin.x) + (ui_.statsOpen() ? (oldWidth-newWidth)/2 : dashboardOffset_-dashboardPixels), static_cast<int>(origin.y) + oldHeight - newHeight);
    if (!ui_.statsOpen()) dashboardOffset_=dashboardPixels;
    statsWindow_ = ui_.statsOpen();
    desktop::keepInWorkArea(GetWindowHandle());
    drag_.cancel();
}

UIAction Game::keyboardInput() {
    if (smoke_ || !IsWindowFocused()) return {};
    // Recovery keys mirror the taskbar-companion convention and stay active when shortcuts are off.
    const bool shift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
    if (shift && IsKeyPressed(KEY_F12)) return {UIActionType::ResetPosition};
    if (shift && IsKeyPressed(KEY_F11)) return {UIActionType::ResetScale};
    if (ui_.onboardingOpen()) {
        if (IsKeyPressed(KEY_ESCAPE)) ui_.onboardingBack();
        return {};
    }
    if (IsKeyPressed(KEY_ESCAPE)) {
        if (ui_.statsOpen()) ui_.backFromStats(); else ui_.closePanels();
        return {};
    }
    if (!preferences_.hotkeys) return {};
    if (IsKeyPressed(KEY_U)) ui_.toggleStats();
    if (IsKeyPressed(KEY_R)) return {UIActionType::UpgradeRod};
    if (IsKeyPressed(KEY_F)) return {UIActionType::UpgradeBox};
    if (ui_.statsOpen()) return {};
    if (IsKeyPressed(KEY_B)) ui_.toggleBox();
    if (IsKeyPressed(KEY_C)) ui_.toggleCollection();
    if (IsKeyPressed(KEY_S) && ui_.boxOpen() && ui_.selectedSlot()) return {UIActionType::SellStored, *ui_.selectedSlot()};
    if (IsKeyPressed(KEY_L) && ui_.boxOpen() && ui_.selectedSlot()) return {UIActionType::ToggleLock, *ui_.selectedSlot()};
    if (IsKeyPressed(KEY_A)) return {UIActionType::ToggleAutoSell};
    if (IsKeyPressed(KEY_T)) return {UIActionType::ToggleTopmost};
    if (IsKeyPressed(KEY_D)) return {UIActionType::Dock};
    return {};
}

bool Game::settleCatch() {
    if (!fishing_.catchSettled() || !fishing_.catchResult()) return false;
    const auto fish = *fishing_.catchResult();
    const auto rarity = fishing_.database().at(fish.speciesId).rarity;
    const bool protect = (preferences_.protectSpecial && rarity == FishRarity::Special) ||
        (preferences_.protectRare && (rarity == FishRarity::Rare || rarity == FishRarity::Epic || rarity == FishRarity::Legendary));
    if (fishing_.shouldAutoSell(autoSellCommon_)) player_.sell(fish);
    else {
        if (player_.boxFull() && preferences_.boxFullAction == 2) {
            // Make room by selling the least valuable unlocked fish; protected fish are never touched.
            std::optional<std::size_t> cheapest;
            const auto& box = player_.fishBox();
            for (std::size_t i = 0; i < box.size(); ++i)
                if (!box[i].locked && (!cheapest || player_.salePrice(box[i]) < player_.salePrice(box[*cheapest]))) cheapest = i;
            if (cheapest) player_.sellStored(*cheapest);
        }
        if (player_.keep(fish)) {
            if (protect && !fish.locked) player_.toggleLock(player_.fishBox().size()-1);
        } else if (preferences_.boxFullAction == 1 && !protect) player_.sell(fish);
        else return false; // Box full: the catch waits on the hook until a slot frees up.
    }
    fishing_.resolveCatch(player_.rodLevel());
    save();
    return true;
}

bool Game::handleAction(UIAction action) {
    switch (action.type) {
    case UIActionType::Close: return false;
    case UIActionType::Minimize: MinimizeWindow(); break;
    case UIActionType::Dock: dock(); save(); break;
    case UIActionType::ResetPosition: dock(true); save(); break;
    case UIActionType::ResetScale: {
        const auto previous = preferences_;
        preferences_.scale = 100; preferences_.opacity = preferences_.idleOpacity = 100;
        applyPreferences(previous); preferencesDirty_ = true; break;
    }
    case UIActionType::OpenSaveFolder:
        if (!smoke_) desktop::openFolder(savePath_.has_parent_path() ? savePath_.parent_path() : savePath_);
        break;
    case UIActionType::SetPreference: {
        const auto previous = preferences_;
        if (preferences_.set(static_cast<PreferenceId>(action.index),action.value)) {
            applyPreferences(previous); preferencesDirty_ = true;
        }
        break;
    }
    case UIActionType::SaveNow: save(); break;
    case UIActionType::ResetPreferences: {
        const auto previous = preferences_;
        preferences_=GamePreferences{}; autoSellCommon_=alwaysOnTop_=false;
        ClearWindowState(FLAG_WINDOW_TOPMOST);
        applyPreferences(previous); preferencesDirty_ = true; break;
    }
    case UIActionType::ToggleTopmost:
        alwaysOnTop_ = !alwaysOnTop_;
        if (alwaysOnTop_) SetWindowState(FLAG_WINDOW_TOPMOST);
        else ClearWindowState(FLAG_WINDOW_TOPMOST);
        save();
        break;
    case UIActionType::ToggleAutoSell: autoSellCommon_ = !autoSellCommon_; save(); break;
    case UIActionType::UpgradeRod: if (player_.upgradeRod()) save(); break;
    case UIActionType::UpgradeBox: if (player_.upgradeBox()) save(); break;
    case UIActionType::UpgradeStat: if (player_.tryUpgrade(static_cast<UpgradeId>(action.index))) save(); break;
    case UIActionType::SellStored: if (player_.sellStored(action.index)) save(); break;
    case UIActionType::SellAll: if (player_.sellAll() > 0) save(); break;
    case UIActionType::ToggleLock: if (player_.toggleLock(action.index)) save(); break;
    case UIActionType::FinishOnboarding:
        if (player_.setName(ui_.onboardingName()) && !player_.name().empty()) {
            ui_.endOnboarding();
            if (!smoke_ && preferences_.startWithWindows) desktop::setStartWithWindows(true);
            save();
        }
        break;
    case UIActionType::None: break;
    }
    return true;
}

void Game::run(bool showStats) {
    if (showStats && !smoke_) ui_.toggleStats();
    const auto applicationDirectory = assets::applicationDirectory(GetApplicationDirectory());
    const auto artwork = DiscordPresence::readArtwork(applicationDirectory / "discord_presence.json");
    std::unique_ptr<DiscordPresence> discord; // Created and torn down as the Discord preference changes.
    auto nextPresenceSample = std::chrono::steady_clock::now();
    auto nextClockSample = std::chrono::steady_clock::now();
    int displayedHour = -1;
    fishing_.setStats(player_.stats());
    fishing_.start(player_.rodLevel());
    int frames = 0, smokeCaught = 0, smokeEscaped = 0, smokeStored = 0, smokeSales = 0;
    int targetFps = preferences_.fps;
    bool running = true;
    // Smoke script: open the Fish Box, select the first slot, press Sell, return to Catch.
    const Vector2 slot0{UI::slotRect(0).x + 12, UI::slotRect(0).y + 12};
    const std::array<Vector2, 4> smokeClicks{{{464, 16}, slot0, {408, 285}, {464, 376}}};
    int smokeStep = -1; // -1 idle, otherwise index into smokeClicks * 2 (press, release)
    Vector2 smokeWidgetPosition{};
    int smokeStatsChecks = 0;
    while (running && !WindowShouldClose() && (!smoke_ || frames < 120)) {
        // "Active" means the player is looking at or pointing at the lake; idle fades and caps apply otherwise.
        const bool active = smoke_ || IsWindowFocused() || drag_.active() || desktop::cursorOverWindow(GetWindowHandle());
        const int backgroundFps = preferences_.backgroundFps ? std::min(preferences_.fps, preferences_.backgroundFps) : preferences_.fps;
        const int desiredFps = IsWindowMinimized() ? 15 : active ? preferences_.fps : backgroundFps;
        if (targetFps != desiredFps) { SetTargetFPS(desiredFps); targetFps = desiredFps; }
        const float dt = smoke_ ? 0.5f : GetFrameTime();
        updateOpacity(dt, active);
        const bool wantDiscord = discordEnabled_ && preferences_.discordPresence;
        if (wantDiscord != static_cast<bool>(discord)) {
            if (wantDiscord) { discord = std::make_unique<DiscordPresence>(kDiscordApplicationId, artwork); nextPresenceSample = std::chrono::steady_clock::now(); }
            else discord.reset();
        }
        if (smoke_ || std::chrono::steady_clock::now() >= nextClockSample) {
            const auto local = smoke_ ? LocalTimeSnapshot{18, 0, 420} : localTimeNow();
            fishing_.setLocalHour(local.hour);
            ui_.setLocalTime(local);
            if (local.hour != displayedHour) {
                TraceLog(LOG_INFO, "LOCAL FISHING: %s, catch chance %d%%", local.label().c_str(), conditionsForHour(local.hour).successPercent);
                displayedHour = local.hour;
            }
            nextClockSample = std::chrono::steady_clock::now() + std::chrono::milliseconds(250);
        }
        if (smoke_ && frames == 90) ui_.closePanels();
        if (smoke_ && frames == 91) { smokeWidgetPosition = GetWindowPosition(); ui_.toggleStats(); }
        if (smoke_ && frames == 110) ui_.backFromStats();
        ui_.update(dt);
        syncWindowSize();
        if (preferencesDirty_ && !IsMouseButtonDown(MOUSE_BUTTON_LEFT)) { save(); preferencesDirty_ = false; }
        if (smoke_ && frames == 92) {
            if (!ui_.statsOpen() || GetScreenWidth() != UI::kStatsWidth || GetScreenHeight() != UI::kStatsHeight)
                throw std::runtime_error("Stats window did not expand");
            ++smokeStatsChecks;
        }
        if (smoke_ && frames == 111) {
            const auto position = GetWindowPosition();
            if (ui_.statsOpen() || GetScreenWidth() != UI::kWidth || GetScreenHeight() != UI::kCompactHeight ||
                position.x != smokeWidgetPosition.x || position.y != smokeWidgetPosition.y)
                throw std::runtime_error("Stats window did not restore the widget");
            ++smokeStatsChecks;
        }
        handleWindowDrag();
        fishing_.setStats(player_.stats());
        const float fishingDt = ui_.onboardingOpen() || (!smoke_ && preferences_.pauseUnfocused && !IsWindowFocused()) ? 0 : dt;
        switch (fishing_.update(fishingDt, player_.rodLevel())) {
        case FishingEvent::Caught:
            lastCatch_ = fishing_.catchResult();
            ui_.notifyCatch(player_.recordCatch(*lastCatch_));
            save();
            ++smokeCaught;
            break;
        case FishingEvent::Escaped: ++smokeEscaped; break;
        case FishingEvent::None: break;
        }
        if (settleCatch()) ++smokeStored;
        if (discord && std::chrono::steady_clock::now() >= nextPresenceSample) {
            discord->update(DiscordPresence::activityFor(fishing_, player_, autoSellCommon_));
            nextPresenceSample = std::chrono::steady_clock::now() + std::chrono::seconds(1);
        }
        UIInput input{ui_.toLogical(GetMousePosition()), IsMouseButtonPressed(MOUSE_BUTTON_LEFT), IsMouseButtonReleased(MOUSE_BUTTON_LEFT),
                      IsMouseButtonPressed(MOUSE_BUTTON_RIGHT), IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT),
                      IsMouseButtonDown(MOUSE_BUTTON_LEFT), GetMouseWheelMove()};
        if (smoke_) {
            input = {};
            // Exercise real UI hit-testing and the same action handler used by the player.
            if (frames < 80 && smokeStep < 0 && !player_.fishBox().empty() && !ui_.boxOpen()) smokeStep = 0;
            if (smokeStep >= 0) {
                input.mouse = smokeClicks[static_cast<std::size_t>(smokeStep / 2)];
                input.pressed = smokeStep % 2 == 0;
                input.released = !input.pressed;
                if (++smokeStep >= static_cast<int>(smokeClicks.size()) * 2) smokeStep = -1;
            }
        }
        const auto keyAction = keyboardInput();
        syncWindowSize();
        BeginDrawing();
        UIAction action = ui_.draw(fishing_, player_, lastCatch_, autoSellCommon_, alwaysOnTop_, input);
        if (!saveWarning_.empty()) {
            const int warningY = GetScreenHeight() - ui_.scaled(27);
            DrawRectangle(ui_.scaled(8), warningY, GetScreenWidth() - ui_.scaled(16), ui_.scaled(27), {18, 29, 43, 255});
            DrawText(saveWarning_.c_str(), ui_.scaled(16), warningY + ui_.scaled(7), ui_.scaled(12), ORANGE);
        }
        EndDrawing();
        if (action.type == UIActionType::None) action = keyAction;
        if (smoke_ && action.type == UIActionType::SellStored) ++smokeSales;
        running = handleAction(action);
        ++frames;
    }
    if (smoke_ && (smokeCaught < 1 || smokeStored < 1 || smokeSales < 1 || player_.money() <= 0 || smokeStatsChecks != 2))
        throw std::runtime_error("Smoke test did not complete catch/store/sell fishing loop");
    if (smoke_) TraceLog(LOG_INFO, "SMOKE PASS: %d frames, caught=%d escaped=%d stored=%d sold=%d money=%lld boxed=%d", frames, smokeCaught, smokeEscaped, smokeStored, smokeSales,
                         static_cast<long long>(player_.money()), static_cast<int>(player_.fishBox().size()));
    save();
}
