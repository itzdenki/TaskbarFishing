#pragma once

// Persisted in this order; only append new entries so older saves keep their meaning.
enum class PreferenceId {
    // Save version 4.
    LockWindow, RememberPosition, ShowHud, CatchNotices, WaterEffects,
    AnimateBackground, ProtectSpecial, PauseUnfocused, Opacity, Fps, DockAlignment,
    // Save version 5.
    ShowTooltips, TwelveHourClock, Hotkeys, DiscordPresence, StartWithWindows,
    ProtectRare, VSync, IdleOpacity, Scale, BackgroundFps, TaskbarGap, BoxFullAction,
    Count
};
constexpr int kPreferenceCount = static_cast<int>(PreferenceId::Count);
constexpr int kLegacyPreferenceCount = static_cast<int>(PreferenceId::DockAlignment) + 1;

struct GamePreferences {
    bool lockWindow = false, rememberPosition = true;
    bool showHud = true, catchNotices = true, waterEffects = true, animateBackground = true;
    bool protectSpecial = true, pauseUnfocused = false;
    int opacity = 100, fps = 60, dockAlignment = 1;
    bool showTooltips = true, twelveHourClock = false, hotkeys = true, discordPresence = true, startWithWindows = false;
    bool protectRare = false, vsync = false;
    int idleOpacity = 100;   // Applied while unfocused with the cursor away; 100 disables the fade.
    int scale = 100;         // Whole-UI scale in percent: 75, 100, 125 or 150.
    int backgroundFps = 0;   // Frame cap while unfocused with the cursor away; 0 keeps fps.
    int taskbarGap = 8;      // Pixels between the docked window and the taskbar.
    int boxFullAction = 0;   // 0 wait, 1 sell the new catch, 2 sell the cheapest unlocked fish.
    bool hasPosition = false;
    int windowX = 0, windowY = 0;

    static bool isToggle(PreferenceId id) {
        switch (id) {
        case PreferenceId::Opacity: case PreferenceId::Fps: case PreferenceId::DockAlignment:
        case PreferenceId::IdleOpacity: case PreferenceId::Scale: case PreferenceId::BackgroundFps:
        case PreferenceId::TaskbarGap: case PreferenceId::BoxFullAction: case PreferenceId::Count:
            return false;
        default: return true;
        }
    }

    int get(PreferenceId id) const {
        switch (id) {
        case PreferenceId::LockWindow: return lockWindow;
        case PreferenceId::RememberPosition: return rememberPosition;
        case PreferenceId::ShowHud: return showHud;
        case PreferenceId::CatchNotices: return catchNotices;
        case PreferenceId::WaterEffects: return waterEffects;
        case PreferenceId::AnimateBackground: return animateBackground;
        case PreferenceId::ProtectSpecial: return protectSpecial;
        case PreferenceId::PauseUnfocused: return pauseUnfocused;
        case PreferenceId::Opacity: return opacity;
        case PreferenceId::Fps: return fps;
        case PreferenceId::DockAlignment: return dockAlignment;
        case PreferenceId::ShowTooltips: return showTooltips;
        case PreferenceId::TwelveHourClock: return twelveHourClock;
        case PreferenceId::Hotkeys: return hotkeys;
        case PreferenceId::DiscordPresence: return discordPresence;
        case PreferenceId::StartWithWindows: return startWithWindows;
        case PreferenceId::ProtectRare: return protectRare;
        case PreferenceId::VSync: return vsync;
        case PreferenceId::IdleOpacity: return idleOpacity;
        case PreferenceId::Scale: return scale;
        case PreferenceId::BackgroundFps: return backgroundFps;
        case PreferenceId::TaskbarGap: return taskbarGap;
        case PreferenceId::BoxFullAction: return boxFullAction;
        case PreferenceId::Count: break;
        }
        return 0;
    }

    // Rejects out-of-range values without changing anything, so save files can be validated.
    bool set(PreferenceId id, int value) {
        if (isToggle(id) && value != 0 && value != 1) return false;
        switch (id) {
        case PreferenceId::LockWindow: lockWindow = value; break;
        case PreferenceId::RememberPosition: rememberPosition = value; break;
        case PreferenceId::ShowHud: showHud = value; break;
        case PreferenceId::CatchNotices: catchNotices = value; break;
        case PreferenceId::WaterEffects: waterEffects = value; break;
        case PreferenceId::AnimateBackground: animateBackground = value; break;
        case PreferenceId::ProtectSpecial: protectSpecial = value; break;
        case PreferenceId::PauseUnfocused: pauseUnfocused = value; break;
        case PreferenceId::Opacity: if (value < 30 || value > 100) return false; opacity = value; break;
        case PreferenceId::Fps: if (value != 30 && value != 60 && value != 120) return false; fps = value; break;
        case PreferenceId::DockAlignment: if (value < 0 || value > 2) return false; dockAlignment = value; break;
        case PreferenceId::ShowTooltips: showTooltips = value; break;
        case PreferenceId::TwelveHourClock: twelveHourClock = value; break;
        case PreferenceId::Hotkeys: hotkeys = value; break;
        case PreferenceId::DiscordPresence: discordPresence = value; break;
        case PreferenceId::StartWithWindows: startWithWindows = value; break;
        case PreferenceId::ProtectRare: protectRare = value; break;
        case PreferenceId::VSync: vsync = value; break;
        case PreferenceId::IdleOpacity: if (value < 30 || value > 100) return false; idleOpacity = value; break;
        case PreferenceId::Scale: if (value != 75 && value != 100 && value != 125 && value != 150) return false; scale = value; break;
        case PreferenceId::BackgroundFps: if (value != 0 && value != 15 && value != 30) return false; backgroundFps = value; break;
        case PreferenceId::TaskbarGap: if (value < 0 || value > 48) return false; taskbarGap = value; break;
        case PreferenceId::BoxFullAction: if (value < 0 || value > 2) return false; boxFullAction = value; break;
        case PreferenceId::Count: return false;
        }
        return true;
    }
};
