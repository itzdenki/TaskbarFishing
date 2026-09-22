#pragma once
#include "fish/Fish.h"
#include <random>
#include <vector>

class FishDatabase {
public:
    static constexpr double kSpecialChance = 0.001;
    FishDatabase();
    const std::vector<FishSpecies>& all() const { return species_; }
    const FishSpecies& at(int id) const;
    FishInstance roll(std::mt19937& random, int rodLevel, float rareBonus = 0) const;
    int price(int id, float weight) const;
    bool valid(const FishInstance& fish) const;
private:
    std::vector<FishSpecies> species_;
};
