#include "save/SaveSystem.h"
#include <fstream>
#include <iomanip>
#include <limits>
#include <locale>
#include <cmath>
#include <cstdlib>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

std::filesystem::path SaveSystem::defaultPath() {
#ifdef _WIN32
    if (const wchar_t* local = _wgetenv(L"LOCALAPPDATA")) return std::filesystem::path(local) / "TaskbarFishing" / "save.txt";
#endif
    return std::filesystem::current_path() / "save.txt";
}

bool SaveSystem::read(const std::filesystem::path& path, SaveData& data, const FishDatabase& database) {
    std::ifstream input(path);
    input.imbue(std::locale::classic());
    if (!input) return false;
    SaveData candidate; // Commit only after the entire file has passed validation.
    auto key = [&](const char* expected) {
        std::string token;
        return static_cast<bool>(input >> token) && token == expected;
    };
    auto readFish = [&](FishInstance& fish, bool withLock) {
        if (!(input >> fish.speciesId >> fish.weight >> fish.sellPrice)) return false;
        int locked = 0;
        if (withLock && (!(input >> locked) || (locked != 0 && locked != 1))) return false;
        fish.locked = locked != 0;
        return database.valid(fish);
    };
    int version = 0, autoSell = 0, topmost = 0;
    // Version 1 stored fish as "id weight price"; version 2 appends the lock flag.
    if (!key("TASKBAR_FISHING") || !(input >> version) || (version < 1 || version > 6)) return false;
    if (!key("money") || !(input >> candidate.player.money_) || candidate.player.money_ < 0 || candidate.player.money_ > 9'999'999'999) return false;
    if (!key("rod") || !(input >> candidate.player.rodLevel_) || candidate.player.rodLevel_ < 1 || candidate.player.rodLevel_ > 5) return false;
    if (!key("box") || !(input >> candidate.player.boxLevel_) || candidate.player.boxLevel_ < 1 || candidate.player.boxLevel_ > 5) return false;
    if (version >= 3) {
        int count = 0;
        if (!key("stats") || !(input >> count) || count != static_cast<int>(kStatCount)) return false;
        for (std::size_t i = 0; i < kStatCount; ++i) {
            if (!key(kUpgrades[i + 8].key) || !(input >> candidate.player.statRanks_[i]) ||
                candidate.player.statRanks_[i] < 0 || candidate.player.statRanks_[i] > 3) return false;
        }
        if (!candidate.player.validUpgrades()) return false;
    }
    if (!key("settings") || !(input >> autoSell >> topmost) || (autoSell != 0 && autoSell != 1) || (topmost != 0 && topmost != 1)) return false;
    candidate.autoSellCommon = autoSell != 0;
    candidate.alwaysOnTop = topmost != 0;
    if (version >= 4) {
        // Version 4 stored the first eleven preferences; version 5 prefixes the list with its length
        // so preferences appended later keep their defaults when an older file is read.
        if (!key("preferences")) return false;
        int count = kLegacyPreferenceCount;
        if (version >= 5 && (!(input >> count) || count < kLegacyPreferenceCount || count > kPreferenceCount)) return false;
        for (int i=0;i<count;++i) {
            int value=0;
            if (!(input>>value) || !candidate.preferences.set(static_cast<PreferenceId>(i),value)) return false;
        }
        int hasPosition=0;
        if (!(input>>hasPosition>>candidate.preferences.windowX>>candidate.preferences.windowY) ||
            (hasPosition!=0 && hasPosition!=1)) return false;
        if (candidate.preferences.windowX < -100000 || candidate.preferences.windowX > 100000 ||
            candidate.preferences.windowY < -100000 || candidate.preferences.windowY > 100000) return false;
        candidate.preferences.hasPosition=hasPosition!=0;
    }
    if (version >= 6) {
        int nameBytes = 0;
        if (!key("name") || !(input >> nameBytes) || nameBytes < 0 || nameBytes > kMaxNameChars * 4) return false;
        if (nameBytes > 0) {
            if (input.get() != ' ') return false;
            std::string name(static_cast<std::size_t>(nameBytes), '\0');
            if (!input.read(name.data(), nameBytes) || !candidate.player.setName(std::move(name))) return false;
        }
    }
    int count = 0;
    if (!key("stored") || !(input >> count) || count < 0 || count > candidate.player.capacity()) return false;
    for (int i = 0; i < count; ++i) {
        FishInstance fish;
        if (!readFish(fish, version >= 2)) return false;
        candidate.player.fishBox_.push_back(fish);
    }
    // The original six IDs remain stable; older saves have no entries for specials.
    if (!key("dex") || !(input >> count) || (count != 6 && count != static_cast<int>(database.all().size()))) return false;
    for (int i = 0; i < count; ++i) {
        int id = -1, discovered = 0;
        float record = 0;
        if (!(input >> id >> discovered >> record) || id != i || (discovered != 0 && discovered != 1) || !std::isfinite(record)) return false;
        const auto& species = database.at(id);
        if (discovered ? (record < species.minWeight || record > species.maxWeight) : record != 0) return false;
        if (discovered) candidate.player.collection_[id] = {true, record};
    }
    int pending = 0;
    if (!key("pending") || !(input >> pending) || (pending != 0 && pending != 1)) return false;
    if (pending) {
        FishInstance fish;
        if (!readFish(fish, false)) return false;
        candidate.pendingCatch = fish;
    }
    if (!key("END")) return false;
    input >> std::ws;
    if (!input.eof()) return false;
    auto recorded = [&](const FishInstance& fish) {
        const auto entry = candidate.player.progress(fish.speciesId);
        return entry.discovered && entry.recordWeight >= fish.weight;
    };
    for (const auto& fish : candidate.player.fishBox_) if (!recorded(fish)) return false;
    if (candidate.pendingCatch && !recorded(*candidate.pendingCatch)) return false;
    data = std::move(candidate);
    return true;
}

LoadStatus SaveSystem::load(const std::filesystem::path& path, SaveData& data, const FishDatabase& database, std::string& error) {
    error.clear();
    try {
        if (read(path, data, database)) return LoadStatus::Loaded;
        auto backup = path; backup += ".bak";
        if (read(backup, data, database)) return LoadStatus::RecoveredBackup;
        if (!std::filesystem::exists(path) && !std::filesystem::exists(backup)) return LoadStatus::Missing;
        error = "Save unreadable. Progress will not be saved.";
    } catch (const std::exception& exception) {
        error = exception.what();
    }
    return LoadStatus::Invalid;
}

bool SaveSystem::save(const std::filesystem::path& path, const SaveData& data, const FishDatabase& database, std::string& error) {
    error.clear();
    try {
        if (path.has_parent_path()) std::filesystem::create_directories(path.parent_path());
        auto temp = path; temp += ".tmp";
        {
            std::ofstream output(temp, std::ios::trunc);
            output.imbue(std::locale::classic());
            output << std::setprecision(std::numeric_limits<float>::max_digits10);
            output << "TASKBAR_FISHING 6\nmoney " << data.player.money()
                   << "\nrod " << data.player.rodLevel() << "\nbox " << data.player.boxLevel()
                   << "\nstats " << kStatCount << '\n';
            for (std::size_t i = 8; i < kUpgradeCount; ++i)
                output << kUpgrades[i].key << ' ' << data.player.upgradeRank(kUpgrades[i].id) << '\n';
            const auto& p=data.preferences;
            output << "settings " << data.autoSellCommon << ' ' << data.alwaysOnTop
                   << "\npreferences " << kPreferenceCount;
            for (int i = 0; i < kPreferenceCount; ++i) output << ' ' << p.get(static_cast<PreferenceId>(i));
            output << ' ' << p.hasPosition << ' ' << p.windowX << ' ' << p.windowY
                   << "\nname " << data.player.name().size();
            if (!data.player.name().empty()) output << ' ' << data.player.name();
            output << "\nstored " << data.player.fishBox().size() << '\n';
            for (const auto& fish : data.player.fishBox()) output << fish.speciesId << ' ' << fish.weight << ' ' << fish.sellPrice << ' ' << fish.locked << '\n';
            output << "dex " << database.all().size() << '\n';
            for (std::size_t id = 0; id < database.all().size(); ++id) {
                const auto entry = data.player.progress(static_cast<int>(id));
                output << id << ' ' << entry.discovered << ' ' << entry.recordWeight << '\n';
            }
            output << "pending " << data.pendingCatch.has_value() << '\n';
            if (data.pendingCatch) output << data.pendingCatch->speciesId << ' ' << data.pendingCatch->weight << ' ' << data.pendingCatch->sellPrice << '\n';
            output << "END\n";
            output.flush();
            if (!output) { error = "Could not write save file."; return false; }
            output.close();
            if (!output) { error = "Could not close save file."; return false; }
        }
        SaveData verified;
        if (!read(temp, verified, database)) { error = "Save validation failed."; return false; }
        // Preserve only a valid prior save; a corrupt primary must not overwrite a good backup.
        if (read(path, verified, database)) {
            auto backup = path; backup += ".bak";
            std::filesystem::copy_file(path, backup, std::filesystem::copy_options::overwrite_existing);
        }
#ifdef _WIN32
        // Same-directory replacement keeps the old save intact if the write is interrupted.
        if (!MoveFileExW(temp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
            error = "Could not replace save file.";
            return false;
        }
#else
        std::filesystem::rename(temp, path);
#endif
        return true;
    } catch (const std::exception& exception) {
        error = exception.what();
        return false;
    }
}
