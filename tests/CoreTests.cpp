#include "fishing/FishingSystem.h"
#include "player/Player.h"
#include "save/SaveSystem.h"
#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <chrono>
#include <fstream>
#include <sstream>

void check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void testFishing() {
    FishingSystem fishing(42);
    check(fishing.state() == FishingState::Idle, "starts idle");
    fishing.start();
    fishing.update(0.65f);
    check(fishing.state() == FishingState::Waiting, "casting to waiting");
    check(fishing.remaining() >= 5 && fishing.remaining() <= 12, "level 1 wait range");
    fishing.update(fishing.remaining());
    check(fishing.state() == FishingState::FishBiting && fishing.catchResult().has_value(), "bite rolls a fish");
    fishing.update(0.5f);
    check(fishing.state() == FishingState::Reeling, "bite to reeling");
    // Reeling ends in exactly one Caught or Escaped event; drive cycles until a fish is landed.
    auto landOne = [](FishingSystem& system, int rodLevel) {
        for (int cycle = 0; cycle < 500; ++cycle) {
            const auto event = system.update(system.remaining(), rodLevel);
            if (event == FishingEvent::Caught) return true;
            if (event == FishingEvent::Escaped) {
                check(system.state() == FishingState::Escaped && system.catchResult().has_value(), "escaped fish shown briefly");
                check(system.update(FishingSystem::kEscapedDisplay, rodLevel) == FishingEvent::None, "escape resolves silently");
                check(system.state() == FishingState::Casting && !system.catchResult(), "escape starts a new cast without a fish");
                check(system.elapsed() == 0 && system.remaining() > 0, "escape restarts the full casting animation");
            }
        }
        return false;
    };
    check(landOne(fishing, 1), "eventually lands a fish");
    check(!fishing.catchSettled(), "landed fish is shown before it is stored");
    check(fishing.update(100) == FishingEvent::None, "catch event must not repeat");
    check(fishing.state() == FishingState::Caught && fishing.catchSettled(), "caught waits for the game to store it");
    fishing.resolveCatch();
    check(!fishing.catchResult() && fishing.state() == FishingState::Casting, "stored catch starts a new cast");
    check(fishing.elapsed() == 0, "successful catch restarts casting from the first frame");
    fishing.update(.3f);
    check(fishing.state() == FishingState::Casting, "recast is displayed before waiting");
    fishing.update(fishing.remaining());
    check(fishing.state() == FishingState::Waiting, "recast completes before next waiting period");
    // User-specified schedule: verify both rod levels against every hourly probability.
    constexpr std::array<int, 24> expected{{42,38,34,32,40,58,78,85,72,60,48,38,30,28,32,42,58,72,88,82,68,55,48,44}};
    for (int hour = 0; hour < 24; ++hour) {
        check(conditionsForHour(hour).successPercent == expected[hour], "exact hourly table");
        for (int rod : {1, 5}) {
            FishingSystem trials(99 + hour);
            trials.setLocalHour(hour);
            trials.start(rod);
            int caught = 0, escaped = 0;
            while (caught + escaped < 4000) {
                const auto event = trials.update(trials.remaining(), rod);
                if (event == FishingEvent::Caught) { ++caught; trials.resolveCatch(rod); }
                else if (event == FishingEvent::Escaped) ++escaped;
            }
            check(std::abs(caught / 4000.0f - expected[hour] / 100.0f) < 0.035f,
                  "actual success rate matches hour without rod or rarity multipliers");
        }
    }
    FishingSystem boundary(13);
    boundary.setLocalHour(18); boundary.start(); boundary.update(0.65f);
    boundary.update(boundary.remaining()); boundary.update(0.5f);
    check(boundary.state() == FishingState::Reeling, "prepared attempt before an hour change");
    boundary.setLocalHour(13); boundary.update(1);
    check(boundary.lastAttemptHour() == 13, "use realtime hour at the end of reeling");
    boundary.setLocalHour(18);
    check(boundary.lastAttemptHour() == 13, "completed attempt remembers its original odds");
    FishingSystem fast(42), slow(42);
    fast.start(5); slow.start(1);
    fast.update(0.65f, 5); slow.update(0.65f, 1);
    check(std::abs(fast.remaining() - slow.remaining() * 0.6f) < 0.001f, "rod speed scales wait");
    FishingSystem whole(19), chunks(19);
    whole.start(); chunks.start();
    whole.update(7.0f);
    for (int i = 0; i < 700; ++i) chunks.update(0.01f);
    check(whole.state() == chunks.state() && std::abs(whole.elapsed() - chunks.elapsed()) < 0.002f, "delta time invariance");
    const float elapsed = whole.elapsed();
    whole.update(-1); whole.update(std::numeric_limits<float>::infinity());
    check(whole.elapsed() == elapsed, "ignore invalid delta time");
    FishingSystem automatic(7);
    automatic.restoreCatch({1, 2.0f, 14});
    check(automatic.shouldAutoSell(true) && !automatic.shouldAutoSell(false), "auto sell applies to common only when enabled");
    automatic.restoreCatch({2, 2.0f, 24});
    check(!automatic.shouldAutoSell(true), "uncommon fish still wait for a decision");
    automatic.restoreCatch({4, 2.0f, 120});
    check(!automatic.shouldAutoSell(true), "rare fish are never automatically sold");
    automatic.restoreCatch({5, 1.0f, 2});
    check(automatic.shouldAutoSell(true), "old boot follows common auto sell rule");
    automatic.resolveCatch();
    check(!automatic.shouldAutoSell(true), "no automatic sale outside caught state");
}

void testFish() {
    FishDatabase database;
    std::mt19937 random(123);
    std::vector<int> counts(database.all().size());
    for (int i = 0; i < 10000; ++i) {
        const auto fish = database.roll(random, 1 + i % 5);
        check(database.valid(fish), "rolled fish has legal weight and price");
        ++counts.at(fish.speciesId);
    }
    for (int id = 0; id < 6; ++id) check(counts[id] > 0, "all original species reachable");
    check(counts[4] < counts[0], "rare species less frequent than common");
    std::mt19937 specialRandom(709), upgradedRandom(709);
    int specials = 0;
    std::array<int, 5> specialCounts{};
    for (int i = 0; i < 1000000; ++i) {
        const auto fish = database.roll(specialRandom, 1);
        const auto upgraded = database.roll(upgradedRandom, 5, .6f);
        const bool special = database.at(fish.speciesId).rarity == FishRarity::Special;
        check(special == (database.at(upgraded.speciesId).rarity == FishRarity::Special), "upgrades do not inflate special odds");
        if (special) { ++specials; ++specialCounts.at(fish.speciesId - 6); }
    }
    check(specials > 700 && specials < 1300, "special rate remains near one in one thousand");
    for (int count : specialCounts) check(count > 0, "all five specials are reachable");
    check(specialCounts[4] < specialCounts[0], "moonlit chest is rarer than driftwood parcel");
    Player specialPlayer;
    FishingSystem specialFishing;
    for (int id = 6; id < 11; ++id) {
        const auto& species = database.at(id);
        const FishInstance item{id, species.minWeight, database.price(id, species.minWeight)};
        check(database.valid(item) && specialPlayer.recordCatch(item).newSpecies && specialPlayer.keep(item), "special behaves like stored fish");
        specialFishing.restoreCatch(item);
        check(!specialFishing.shouldAutoSell(true), "special is excluded from auto sell common");
    }
    specialPlayer.toggleLock(4);
    check(specialPlayer.sellAll() > 0 && specialPlayer.fishBox().size() == 1, "sell all respects protected specials");
}

void testPlayer() {
    Player player;
    check(player.name().empty(), "starts unnamed");
    check(player.setName("Denki") && player.name()=="Denki", "stores a display name");
    check(!player.setName(std::string(17,'a')) && player.name()=="Denki", "names longer than 16 characters are rejected");
    check(!player.setName("bad\nname") && player.name()=="Denki", "control characters are rejected");
    check(player.setName("  Angler  ") && player.name()=="Angler", "outer spaces are trimmed");
    check(!validPlayerName("", false) && validPlayerName("A"), "onboarding requires a name");
    check(player.setName("") && player.name().empty(), "empty names stay valid for legacy saves");
    const FishInstance carp{1, 2.0f, 14};
    check(player.money() == 0 && player.capacity() == 30, "initial economy");
    check(!player.upgradeRod() && !player.upgradeBox(), "no free upgrades");
    for (int i = 0; i < 30; ++i) check(player.keep(carp), "keep within capacity");
    check(!player.keep(carp) && player.fishBox().size() == 30, "full box rejects keep without losing stored fish");
    check(!player.sellStored(30), "invalid stored index");
    check(player.sellStored(2) && player.money() == 14 && player.fishBox().size() == 29, "sell stored credits once and frees slot");
    check(player.keep(carp), "slot can be reused");
    for (int i = 0; i < 400; ++i) player.sell(carp);
    for (int level = 2; level <= 5; ++level) {
        auto before = player.money();
        const int rodCost = player.rodUpgradeCost();
        check(player.upgradeRod() && player.rodLevel() == level && player.money() == before - rodCost, "rod upgrade deducts exact cost");
        before = player.money();
        const int boxCost = player.boxUpgradeCost();
        check(player.upgradeBox() && player.boxLevel() == level && player.money() == before - boxCost, "box upgrade deducts exact cost");
    }
    check(player.capacity() == 70 && !player.upgradeRod() && !player.upgradeBox(), "level caps");

    Player box;
    const FishInstance bass{2, 3.0f, 36};
    for (int i = 0; i < 4; ++i) box.keep(i % 2 == 0 ? carp : bass);
    check(!box.toggleLock(4) && box.toggleLock(1) && box.fishBox()[1].locked, "lock flags a stored fish");
    check(!box.sellStored(1) && box.fishBox().size() == 4 && box.money() == 0, "locked fish cannot be sold");
    check(box.boxValue(false) == 14 + 36 + 14 + 36 && box.boxValue(true) == 14 + 14 + 36, "box value with and without locked fish");
    check(box.sellAll() == 64 && box.money() == 64, "sell all earns the unlocked value");
    check(box.fishBox().size() == 1 && box.fishBox()[0].locked && box.fishBox()[0].speciesId == 2, "sell all keeps locked fish");
    check(box.sellAll() == 0 && box.toggleLock(0) && box.sellStored(0) && box.fishBox().empty(), "unlocked fish sells normally");
}

void testCollection() {
    Player player;
    auto milestone = player.recordCatch({1, 2.0f, 14});
    check(milestone.newSpecies && milestone.newRecord, "first catch discovers species and sets record");
    milestone = player.recordCatch({1, 2.0f, 14});
    check(!milestone.newSpecies && !milestone.newRecord, "equal weight is not a record");
    milestone = player.recordCatch({1, 1.0f, 7});
    check(!milestone.newRecord && player.progress(1).recordWeight == 2, "smaller catch cannot lower record");
    milestone = player.recordCatch({1, 3.0f, 21});
    check(!milestone.newSpecies && milestone.newRecord, "larger catch improves record");
    player.sell({1, 3.0f, 21});
    check(player.progress(1).discovered && player.progress(1).recordWeight == 3 && player.fishBox().empty(), "collection independent of storage and sales");
    check(!player.progress(0).discovered, "uncaught species stays unknown");
}

void testSave() {
    struct TemporarySave {
        std::filesystem::path directory = std::filesystem::temp_directory_path() / ("taskbar_fishing_test_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        std::filesystem::path path = directory / "test.save";
        TemporarySave() { std::filesystem::create_directories(directory); }
        ~TemporarySave() {
            std::error_code ignored;
            for (const char* suffix : {"", ".tmp", ".bak"}) std::filesystem::remove(std::filesystem::path(path.string() + suffix), ignored);
            std::filesystem::remove(directory, ignored);
        }
    } temp;
    FishDatabase database;
    std::string error;
    SaveData data, loaded;
    check(SaveSystem::load(temp.path, loaded, database, error) == LoadStatus::Missing, "missing save starts fresh");
    FishInstance carp{1, 3.0f, 21};
    data.player.recordCatch(carp);
    data.player.keep(carp);
    for (int i = 0; i < 20; ++i) data.player.sell(carp);
    data.player.upgradeRod(); data.player.upgradeBox();
    data.pendingCatch = FishInstance{4, 2.0f, 120};
    data.player.recordCatch(*data.pendingCatch);
    data.player.toggleLock(0);
    data.autoSellCommon = true;
    data.alwaysOnTop = true;
    data.preferences.opacity=70;
    data.preferences.fps=30;
    data.preferences.showHud=false;
    data.preferences.lockWindow=true;
    data.preferences.hasPosition=true;
    data.preferences.windowX=-420;
    data.preferences.windowY=250;
    data.preferences.scale=125;
    data.preferences.idleOpacity=40;
    data.preferences.backgroundFps=15;
    data.preferences.taskbarGap=24;
    data.preferences.boxFullAction=2;
    data.preferences.twelveHourClock=true;
    data.preferences.hotkeys=false;
    data.preferences.discordPresence=false;
    data.preferences.protectRare=true;
    data.preferences.vsync=true;
    check(data.player.setName("Denki"), "player name is stored on the save");
    check(SaveSystem::save(temp.path, data, database, error), "write save");
    check(SaveSystem::load(temp.path, loaded, database, error) == LoadStatus::Loaded, "load save");
    check(loaded.player.money() == data.player.money() && loaded.player.rodLevel() == 2 && loaded.player.boxLevel() == 2, "economy survives restart");
    check(loaded.player.name() == "Denki", "player name survives restart");
    check(loaded.player.fishBox().size() == 1 && loaded.player.fishBox()[0].weight == 3 && loaded.player.fishBox()[0].sellPrice == 21, "stored fish roundtrip");
    check(loaded.player.fishBox()[0].locked, "lock flag roundtrip");
    check(loaded.preferences.opacity==70 && loaded.preferences.fps==30 && !loaded.preferences.showHud &&
          loaded.preferences.lockWindow && loaded.preferences.windowX==-420, "window and display preferences survive restart");
    check(loaded.preferences.scale==125 && loaded.preferences.idleOpacity==40 && loaded.preferences.backgroundFps==15 &&
          loaded.preferences.taskbarGap==24 && loaded.preferences.boxFullAction==2 && loaded.preferences.twelveHourClock &&
          !loaded.preferences.hotkeys && !loaded.preferences.discordPresence && loaded.preferences.protectRare && loaded.preferences.vsync,
          "version 5 preferences survive restart");
    for (int i = 0; i < kPreferenceCount; ++i) check(loaded.preferences.get(static_cast<PreferenceId>(i)) == data.preferences.get(static_cast<PreferenceId>(i)), "every preference id roundtrips");
    GamePreferences bounds;
    check(!bounds.set(PreferenceId::Scale, 110) && !bounds.set(PreferenceId::Fps, 45) && !bounds.set(PreferenceId::IdleOpacity, 20) &&
          !bounds.set(PreferenceId::BackgroundFps, 60) && !bounds.set(PreferenceId::TaskbarGap, 49) && !bounds.set(PreferenceId::BoxFullAction, 3) &&
          !bounds.set(PreferenceId::VSync, 2) && bounds.scale == 100 && bounds.fps == 60, "invalid preference values are rejected without changes");
    check(bounds.set(PreferenceId::Opacity, 30) && bounds.set(PreferenceId::Scale, 75) && bounds.set(PreferenceId::Fps, 120), "new preference ranges accepted");
    {
        // Version 1 saves (before lock flags) must still load, with every stored fish unlocked.
        std::ofstream output(temp.path);
        output << "TASKBAR_FISHING 1\nmoney 5\nrod 1\nbox 1\nsettings 0 0\nstored 1\n1 3 21\ndex 6\n0 0 0\n1 1 3\n2 0 0\n3 0 0\n4 0 0\n5 0 0\npending 0\nEND\n";
    }
    check(SaveSystem::load(temp.path, loaded, database, error) == LoadStatus::Loaded && loaded.player.fishBox().size() == 1 && !loaded.player.fishBox()[0].locked, "version 1 save still loads");
    check(SaveSystem::save(temp.path, data, database, error) && SaveSystem::load(temp.path, loaded, database, error) == LoadStatus::Loaded, "rewrite as current version");
    std::filesystem::remove(std::filesystem::path(temp.path.string() + ".bak")); // The corruption checks below expect no backup yet.
    check(loaded.player.progress(4).discovered && loaded.player.progress(4).recordWeight == 2 && !loaded.player.progress(0).discovered, "collection roundtrip");
    check(loaded.autoSellCommon && loaded.alwaysOnTop && loaded.pendingCatch->speciesId == 4, "settings and unresolved catch survive restart");
    FishingSystem resumed(12);
    check(resumed.restoreCatch(*loaded.pendingCatch) && resumed.state() == FishingState::Caught && resumed.catchSettled(), "restored catch is ready to be stored immediately");
    check(resumed.update(30) == FishingEvent::None && resumed.state() == FishingState::Caught, "restored catch waits without duplication");
    check(!resumed.restoreCatch({99, 1, 5}), "reject invalid restored fish");
    std::ifstream original(temp.path);
    const std::string validText((std::istreambuf_iterator<char>(original)), {});
    original.close();
    auto eraseLine=[&](std::string& text, const char* key){
        const auto start=text.find(key);
        check(start!=std::string::npos, "legacy field exists");
        text.erase(start, text.find('\n',start)-start+1);
    };
    {
        // Version 4 stored exactly eleven preferences without a count; newer preferences must default.
        const auto v4Path=temp.directory / "legacy-v4.txt";
        auto v4=validText;
        v4.replace(v4.find("TASKBAR_FISHING 6"),std::string("TASKBAR_FISHING 6").size(),"TASKBAR_FISHING 4");
        eraseLine(v4,"name ");
        const auto start=v4.find("preferences ");
        v4.replace(start,v4.find('\n',start)-start,"preferences 1 0 1 1 1 1 1 0 70 30 2 1 -420 250");
        { std::ofstream out(v4Path); out << v4; }
        SaveData fromV4;
        check(SaveSystem::load(v4Path,fromV4,database,error)==LoadStatus::Loaded, "version 4 preferences still load");
        check(fromV4.player.name().empty(), "version 4 saves have no stored name");
        check(fromV4.preferences.lockWindow && !fromV4.preferences.rememberPosition && fromV4.preferences.opacity==70 &&
              fromV4.preferences.fps==30 && fromV4.preferences.dockAlignment==2 && fromV4.preferences.windowX==-420, "version 4 values are kept");
        check(fromV4.preferences.scale==100 && fromV4.preferences.idleOpacity==100 && fromV4.preferences.hotkeys &&
              fromV4.preferences.discordPresence && fromV4.preferences.boxFullAction==0 && fromV4.preferences.taskbarGap==8, "preferences added later use defaults for version 4 saves");
        auto shortList=validText;
        const auto listStart=shortList.find("preferences ");
        shortList.replace(listStart,shortList.find('\n',listStart)-listStart,"preferences 12 1 0 1 1 1 1 1 0 70 30 2 1 1 -420 250");
        { std::ofstream out(v4Path); out << shortList; }
        check(SaveSystem::load(v4Path,fromV4,database,error)==LoadStatus::Loaded && fromV4.preferences.showTooltips && fromV4.preferences.scale==100,
              "shorter version 5 lists load with defaults for the remaining preferences");
        shortList.replace(shortList.find("preferences 12"),std::string("preferences 12").size(),"preferences 99");
        { std::ofstream out(v4Path); out << shortList; }
        check(SaveSystem::load(v4Path,fromV4,database,error)==LoadStatus::Invalid, "oversized preference lists are rejected");
    }
    {
        const auto legacyPath=temp.directory / "legacy-v3.txt";
        const auto dex=validText.find("dex "), pending=validText.find("pending ");
        std::ofstream legacy(legacyPath);
        std::string header=validText.substr(0,dex);
        header.replace(header.find("TASKBAR_FISHING 6"),std::string("TASKBAR_FISHING 6").size(),"TASKBAR_FISHING 3");
        const auto preferences=header.find("preferences ");
        header.erase(preferences,header.find('\n',preferences)-preferences+1);
        const auto name=header.find("name ");
        header.erase(name,header.find('\n',name)-name+1);
        legacy << header << "dex 6\n0 0 0\n1 1 3\n2 0 0\n3 0 0\n4 1 2\n5 0 0\n" << validText.substr(pending);
        legacy.close();
        SaveData migrated;
        check(SaveSystem::load(legacyPath,migrated,database,error)==LoadStatus::Loaded && migrated.player.money()==data.player.money(), "six-species v3 saves migrate without losing money");
        check(!migrated.player.progress(10).discovered, "new specials begin undiscovered in old saves");
        check(!migrated.preferences.lockWindow && migrated.preferences.opacity==100 && migrated.preferences.fps==60, "v3 saves receive usable preference defaults");
        const FishInstance chest{10,2,database.price(10,2)};
        migrated.player.recordCatch(chest);migrated.player.keep(chest);migrated.player.toggleLock(1);
        migrated.pendingCatch=chest;
        check(SaveSystem::save(legacyPath,migrated,database,error), "special save writes");
        SaveData roundtrip;
        check(SaveSystem::load(legacyPath,roundtrip,database,error)==LoadStatus::Loaded && roundtrip.player.fishBox()[1].locked && roundtrip.pendingCatch->speciesId==10, "special inventory, protection, pending catch and collection survive restart");
    }
    const auto originalMoney = loaded.player.money();
    const auto reject = [&](std::string text) {
        { std::ofstream output(temp.path); output << text; }
        check(SaveSystem::load(temp.path, loaded, database, error) == LoadStatus::Invalid, "reject malformed save");
        check(loaded.player.money() == originalMoney, "failed load must not partially change player");
    };
    reject("TASKBAR_FISHING 1\nmoney 50\n");
    auto invalidPreferences=validText;
    const auto preferenceStart=invalidPreferences.find("preferences ");
    invalidPreferences.replace(preferenceStart,invalidPreferences.find('\n',preferenceStart)-preferenceStart,"preferences 0 1 1 1 1 1 1 0 0 60 1 0 0 0");
    reject(invalidPreferences);
    auto bad = validText; bad.replace(bad.find("rod 2"), 5, "rod 999"); reject(bad);
    bad = validText; bad.replace(bad.find("stored 1"), 8, "stored 9999999"); reject(bad);
    bad = validText; bad.replace(bad.find("1 3 21"), 6, "1 3 999"); reject(bad);
    bad = validText; bad.replace(bad.find("1 3 21"), 6, "1 nan 21"); reject(bad);
    auto badName=validText;
    const auto nameStart=badName.find("name ");
    badName.replace(nameStart,badName.find('\n',nameStart)-nameStart,"name 17 aaaaaaaaaaaaaaaaa");
    reject(badName);
    reject(validText + "unexpected trailing data");
    check(SaveSystem::save(temp.path, data, database, error), "replace corrupt file with explicit save");
    data.player.sell(carp);
    check(SaveSystem::save(temp.path, data, database, error), "second save creates backup");
    { std::ofstream output(temp.path); output << "broken"; }
    check(SaveSystem::load(temp.path, loaded, database, error) == LoadStatus::RecoveredBackup && loaded.player.money() == originalMoney, "recover previous valid snapshot");
    check(!SaveSystem::save(temp.path / "child.save", data, database, error), "report unwritable save location");
}

int main() {
    try {
        testFishing(); testFish(); testPlayer(); testCollection(); testSave();
        std::cout << "PASS: fishing, delta time, random fish, storage, upgrades, collection and save/load\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
