#pragma once
#include "fish/Fish.h"
#include "Upgrades.h"
#include <array>
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>

inline constexpr std::array<int, 5> kBoxCapacity{30, 40, 50, 60, 70};
inline constexpr int kMaxNameChars = 16;
int utf8CodepointCount(const std::string& text);
void utf8PopLast(std::string& text);
bool utf8AppendCodepoint(std::string& text, int codepoint, int maxChars = kMaxNameChars);
bool validPlayerName(const std::string& name, bool allowEmpty = true);

struct SpeciesProgress { bool discovered = false; float recordWeight = 0; };
struct CatchMilestone { bool newSpecies = false; bool newRecord = false; };

class Player {
public:
    std::int64_t money() const { return money_; }
    int capacity() const;
    int rodLevel() const { return rodLevel_; }
    int boxLevel() const { return boxLevel_; }
    int rodUpgradeCost() const;
    int boxUpgradeCost() const;
    bool upgradeRod();
    bool upgradeBox();
    int upgradeRank(UpgradeId id) const;
    int upgradeCost(UpgradeId id) const;
    bool upgradeUnlocked(UpgradeId id) const;
    bool canUpgrade(UpgradeId id) const;
    bool tryUpgrade(UpgradeId id);
    bool validUpgrades() const;
    StatsEffects stats() const;
    StatsEffects previewStats(UpgradeId id) const;
    std::int64_t salePrice(const FishInstance& fish) const;
    bool boxFull() const { return fishBox_.size() >= static_cast<std::size_t>(capacity()); }
    const std::vector<FishInstance>& fishBox() const { return fishBox_; }
    bool keep(const FishInstance& fish);
    void sell(const FishInstance& fish);
    // Sells one stored fish. Locked fish are refused.
    bool sellStored(std::size_t index);
    // Sells every unlocked stored fish and returns the money earned.
    std::int64_t sellAll();
    bool toggleLock(std::size_t index);
    // Total sell value of stored fish; unlockedOnly matches what Sell All would earn.
    std::int64_t boxValue(bool unlockedOnly) const;
    CatchMilestone recordCatch(const FishInstance& fish);
    SpeciesProgress progress(int speciesId) const;
    const std::unordered_map<int, SpeciesProgress>& collection() const { return collection_; }
    const std::string& name() const { return name_; }
    bool setName(std::string value);
private:
    friend class SaveSystem;
    std::int64_t money_ = 0;
    int rodLevel_ = 1;
    int boxLevel_ = 1;
    std::array<int, kStatCount> statRanks_{};
    std::vector<FishInstance> fishBox_;
    std::unordered_map<int, SpeciesProgress> collection_;
    std::string name_;
};
