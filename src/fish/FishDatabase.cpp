#include "fish/FishDatabase.h"
#include <algorithm>
#include <cmath>

FishDatabase::FishDatabase() : species_{
    {"Bluegill", FishRarity::Common, 0.15f, 1.2f, 8, 32},
    {"Carp", FishRarity::Common, 1.0f, 5.0f, 7, 28},
    {"Bass", FishRarity::Uncommon, 0.8f, 4.5f, 12, 17},
    {"Catfish", FishRarity::Uncommon, 1.5f, 8.0f, 10, 13},
    {"Golden Carp", FishRarity::Rare, 1.0f, 4.0f, 60, 3},
    {"Old Boot", FishRarity::Common, 0.3f, 1.5f, 2, 7},
    {"Driftwood Parcel", FishRarity::Special, 0.5f, 2.0f, 300, 50},
    {"Fisherman's Gift", FishRarity::Special, 0.5f, 2.5f, 500, 25},
    {"Lucky Tackle Box", FishRarity::Special, 1.0f, 3.0f, 800, 15},
    {"Golden Parcel", FishRarity::Special, 1.0f, 3.0f, 1500, 8},
    {"Moonlit Chest", FishRarity::Special, 2.0f, 5.0f, 2500, 2}
} {}

const FishSpecies& FishDatabase::at(int id) const { return species_.at(static_cast<std::size_t>(id)); }

int FishDatabase::price(int id, float weight) const {
    return std::max(1, static_cast<int>(std::lround(static_cast<float>(at(id).basePrice) * weight)));
}

FishInstance FishDatabase::roll(std::mt19937& random, int rodLevel, float rareBonus) const {
    std::vector<double> chances;
    // Special rarity has its own fixed gate, unaffected by rare-fish upgrades.
    const bool special = std::bernoulli_distribution(kSpecialChance)(random);
    for (const auto& species : species_) {
        if ((species.rarity == FishRarity::Special) != special) { chances.push_back(0); continue; }
        if (special) { chances.push_back(species.catchWeight); continue; }
        const double bonus = species.rarity >= FishRarity::Rare ? (1.0 + 0.12 * (std::clamp(rodLevel, 1, 5) - 1)) * (1.0 + std::clamp(rareBonus, 0.0f, .60f)) : 1.0;
        chances.push_back(species.catchWeight * bonus);
    }
    const int id = std::discrete_distribution<int>(chances.begin(), chances.end())(random);
    const auto& species = at(id);
    // Store hundredths of a kg so the displayed weight is also the price/record weight.
    const float weight = std::round(std::uniform_real_distribution<float>(species.minWeight, species.maxWeight)(random) * 100) / 100;
    return {id, weight, price(id, weight)};
}

bool FishDatabase::valid(const FishInstance& fish) const {
    if (fish.speciesId < 0 || static_cast<std::size_t>(fish.speciesId) >= species_.size() || !std::isfinite(fish.weight)) return false;
    const auto& species = at(fish.speciesId);
    return fish.weight >= species.minWeight && fish.weight <= species.maxWeight && fish.sellPrice == price(fish.speciesId, fish.weight);
}
