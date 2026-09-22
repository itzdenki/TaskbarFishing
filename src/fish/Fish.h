#pragma once
#include <string>

enum class FishRarity { Common, Uncommon, Rare, Epic, Legendary, Special };

struct FishSpecies {
    std::string name;
    FishRarity rarity;
    float minWeight;
    float maxWeight;
    int basePrice;
    float catchWeight;
};

struct FishInstance {
    int speciesId = 0;
    float weight = 0;
    int sellPrice = 0;
    bool locked = false; // Locked fish in the Fish Box are protected from Sell / Sell All.
};

inline const char* rarityName(FishRarity rarity) {
    switch (rarity) {
    case FishRarity::Common: return "Common";
    case FishRarity::Uncommon: return "Uncommon";
    case FishRarity::Rare: return "Rare";
    case FishRarity::Epic: return "Epic";
    case FishRarity::Legendary: return "Legendary";
    case FishRarity::Special: return "Special";
    }
    return "";
}
