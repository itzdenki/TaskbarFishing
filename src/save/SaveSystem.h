#pragma once
#include "player/Player.h"
#include "fish/FishDatabase.h"
#include "platform/GamePreferences.h"
#include <filesystem>
#include <optional>
#include <string>

struct SaveData {
    Player player;
    std::optional<FishInstance> pendingCatch;
    bool autoSellCommon = false;
    bool alwaysOnTop = false;
    GamePreferences preferences;
};

enum class LoadStatus { Missing, Loaded, RecoveredBackup, Invalid };

class SaveSystem {
public:
    static std::filesystem::path defaultPath();
    static LoadStatus load(const std::filesystem::path& path, SaveData& data, const FishDatabase& database, std::string& error);
    static bool save(const std::filesystem::path& path, const SaveData& data, const FishDatabase& database, std::string& error);
private:
    static bool read(const std::filesystem::path& path, SaveData& data, const FishDatabase& database);
};
