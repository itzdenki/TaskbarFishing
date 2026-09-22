#include "player/Player.h"
#include <algorithm>
#include <array>
#include <cmath>

namespace {
std::string trimAscii(std::string value) {
    while (!value.empty() && (value.front() == ' ' || value.front() == '\t')) value.erase(value.begin());
    while (!value.empty() && (value.back() == ' ' || value.back() == '\t')) value.pop_back();
    return value;
}

bool utf8WellFormed(const std::string& text) {
    std::size_t i = 0;
    while (i < text.size()) {
        const auto byte = static_cast<unsigned char>(text[i]);
        int need = 0;
        if (byte < 0x80) need = 1;
        else if ((byte & 0xE0) == 0xC0) need = 2;
        else if ((byte & 0xF0) == 0xE0) need = 3;
        else if ((byte & 0xF8) == 0xF0) need = 4;
        else return false;
        if (i + static_cast<std::size_t>(need) > text.size()) return false;
        for (int n = 1; n < need; ++n) {
            if ((static_cast<unsigned char>(text[i + static_cast<std::size_t>(n)]) & 0xC0) != 0x80) return false;
        }
        i += static_cast<std::size_t>(need);
    }
    return true;
}
}

int utf8CodepointCount(const std::string& text) {
    int count = 0;
    for (unsigned char byte : text) if ((byte & 0xC0) != 0x80) ++count;
    return count;
}

void utf8PopLast(std::string& text) {
    while (!text.empty() && (static_cast<unsigned char>(text.back()) & 0xC0) == 0x80) text.pop_back();
    if (!text.empty()) text.pop_back();
}

bool utf8AppendCodepoint(std::string& text, int codepoint, int maxChars) {
    if (codepoint < 32 || codepoint == 127 || codepoint > 0x10FFFF) return false;
    if (codepoint >= 0xD800 && codepoint <= 0xDFFF) return false;
    if (utf8CodepointCount(text) >= maxChars) return false;
    if (codepoint < 0x80) text += static_cast<char>(codepoint);
    else if (codepoint < 0x800) {
        text += static_cast<char>(0xC0 | (codepoint >> 6));
        text += static_cast<char>(0x80 | (codepoint & 0x3F));
    } else if (codepoint < 0x10000) {
        text += static_cast<char>(0xE0 | (codepoint >> 12));
        text += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
        text += static_cast<char>(0x80 | (codepoint & 0x3F));
    } else {
        text += static_cast<char>(0xF0 | (codepoint >> 18));
        text += static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F));
        text += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
        text += static_cast<char>(0x80 | (codepoint & 0x3F));
    }
    return true;
}

bool validPlayerName(const std::string& name, bool allowEmpty) {
    if (!utf8WellFormed(name)) return false;
    for (unsigned char byte : name) {
        if (byte < 32 || byte == 127) return false;
    }
    const int chars = utf8CodepointCount(name);
    if (chars == 0) return allowEmpty;
    return chars <= kMaxNameChars && name.size() <= static_cast<std::size_t>(kMaxNameChars * 4);
}

bool Player::setName(std::string value) {
    value = trimAscii(std::move(value));
    if (!validPlayerName(value, true)) return false;
    name_ = std::move(value);
    return true;
}

int Player::capacity() const { return kBoxCapacity.at(static_cast<std::size_t>(boxLevel_ - 1)); }
int Player::rodUpgradeCost() const { return rodLevel_ < 5 ? upgradeCost(static_cast<UpgradeId>(rodLevel_ - 1)) : 0; }
int Player::boxUpgradeCost() const { return boxLevel_ < 5 ? upgradeCost(static_cast<UpgradeId>(boxLevel_ + 3)) : 0; }
bool Player::upgradeRod() {
    return rodLevel_ < 5 && tryUpgrade(static_cast<UpgradeId>(rodLevel_ - 1));
}
bool Player::upgradeBox() {
    return boxLevel_ < 5 && tryUpgrade(static_cast<UpgradeId>(4 + boxLevel_ - 1));
}

int Player::upgradeRank(UpgradeId id) const {
    const auto index = static_cast<std::size_t>(id);
    if (index >= kUpgradeCount) return 0;
    if (index < 4) return rodLevel_ >= static_cast<int>(index) + 2 ? 1 : 0;
    if (index < 8) return boxLevel_ >= static_cast<int>(index) - 2 ? 1 : 0;
    return statRanks_[index - 8];
}
int Player::upgradeCost(UpgradeId id) const {
    const auto* def = upgradeDefinition(id);
    if (!def || upgradeRank(id) >= def->maxRank) return 0;
    return def->baseCost * (1 << upgradeRank(id));
}
bool Player::upgradeUnlocked(UpgradeId id) const {
    const auto* def = upgradeDefinition(id);
    if (!def) return false;
    const auto* prerequisite = upgradeDefinition(def->prerequisite);
    return !prerequisite || upgradeRank(prerequisite->id) == prerequisite->maxRank;
}
bool Player::canUpgrade(UpgradeId id) const {
    const auto* def = upgradeDefinition(id);
    return def && upgradeRank(id) < def->maxRank && upgradeUnlocked(id) && money_ >= upgradeCost(id);
}
bool Player::tryUpgrade(UpgradeId id) {
    if (!canUpgrade(id)) return false;
    money_ -= upgradeCost(id);
    const auto index = static_cast<std::size_t>(id);
    if (index < 4) ++rodLevel_;
    else if (index < 8) ++boxLevel_;
    else if (index < kUpgradeCount) ++statRanks_[index - 8];
    return true;
}
bool Player::validUpgrades() const {
    for (const auto& def : kUpgrades) {
        const int rank = upgradeRank(def.id);
        if (rank < 0 || rank > def.maxRank || (rank > 0 && !upgradeUnlocked(def.id))) return false;
    }
    return true;
}
StatsEffects Player::stats() const {
    StatsEffects result;
    for (const auto& def : kUpgrades) {
        const float amount = upgradeRank(def.id) * def.perRank;
        switch (def.kind) {
        case StatKind::Wait: result.waitReduction += amount; break;
        case StatKind::Reel: result.reelReduction += amount; break;
        case StatKind::Accuracy: result.failureReduction += amount; break;
        case StatKind::Rare: result.rareBonus += amount; break;
        case StatKind::Sale: result.saleBonus += amount; break;
        default: break;
        }
    }
    result.waitReduction = std::min(result.waitReduction, .18f);
    result.reelReduction = std::min(result.reelReduction, .45f);
    result.failureReduction = std::min(result.failureReduction, .48f);
    result.rareBonus = std::min(result.rareBonus, .60f);
    result.saleBonus = std::min(result.saleBonus, .30f);
    return result;
}
StatsEffects Player::previewStats(UpgradeId id) const {
    Player preview = *this;
    const auto* def = upgradeDefinition(id);
    if (def && static_cast<std::size_t>(id) >= 8 && upgradeRank(id) < def->maxRank)
        ++preview.statRanks_[static_cast<std::size_t>(id) - 8];
    return preview.stats();
}
std::int64_t Player::salePrice(const FishInstance& fish) const {
    // Keep the base fish value intact for save validation and apply the bonus only on sale.
    int saleRanks = 0;
    for (const auto& def : kUpgrades) if (def.kind == StatKind::Sale) saleRanks += upgradeRank(def.id);
    return std::max<std::int64_t>(1, (static_cast<std::int64_t>(fish.sellPrice) * (1000 + 25 * saleRanks) + 500) / 1000);
}

bool Player::keep(const FishInstance& fish) {
    if (boxFull()) return false;
    fishBox_.push_back(fish);
    return true;
}
void Player::sell(const FishInstance& fish) { money_ = std::min<std::int64_t>(9'999'999'999, money_ + salePrice(fish)); }
bool Player::sellStored(std::size_t index) {
    if (index >= fishBox_.size() || fishBox_[index].locked) return false;
    sell(fishBox_[index]);
    fishBox_.erase(fishBox_.begin() + static_cast<std::ptrdiff_t>(index));
    return true;
}

std::int64_t Player::sellAll() {
    const auto before = money_;
    for (const auto& fish : fishBox_) if (!fish.locked) sell(fish);
    std::erase_if(fishBox_, [](const FishInstance& fish) { return !fish.locked; });
    return money_ - before;
}

bool Player::toggleLock(std::size_t index) {
    if (index >= fishBox_.size()) return false;
    fishBox_[index].locked = !fishBox_[index].locked;
    return true;
}

std::int64_t Player::boxValue(bool unlockedOnly) const {
    std::int64_t total = 0;
    for (const auto& fish : fishBox_) if (!unlockedOnly || !fish.locked) total += salePrice(fish);
    return total;
}

CatchMilestone Player::recordCatch(const FishInstance& fish) {
    auto& entry = collection_[fish.speciesId];
    const CatchMilestone milestone{!entry.discovered, fish.weight > entry.recordWeight};
    entry.discovered = true;
    entry.recordWeight = std::max(entry.recordWeight, fish.weight);
    return milestone;
}

SpeciesProgress Player::progress(int speciesId) const {
    const auto found = collection_.find(speciesId);
    return found == collection_.end() ? SpeciesProgress{} : found->second;
}
