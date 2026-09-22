#pragma once
#include <array>
#include <algorithm>
#include <cstddef>

enum class UpgradeId : std::size_t {
    Rod2, Rod3, Rod4, Rod5, Box2, Box3, Box4, Box5,
    Wait1, Wait2, Wait3, Reel1, Reel2, Reel3,
    Accuracy1, Accuracy2, Accuracy3, Accuracy4, Accuracy5, Accuracy6,
    Rare1, Rare2, Rare3, Rare4, Sale1, Sale2, Sale3, Sale4, Count
};
enum class StatKind { Rod, Box, Wait, Reel, Accuracy, Rare, Sale };
struct UpgradeDefinition {
    UpgradeId id;
    const char* key;
    const char* name;
    StatKind kind;
    int branch, lane, stage, maxRank, baseCost;
    float perRank;
    UpgradeId prerequisite;
};
inline constexpr std::size_t kUpgradeCount = 28, kStatCount = 20;
inline constexpr std::array<UpgradeDefinition, kUpgradeCount> kUpgrades{{
    {UpgradeId::Rod2,"rod_2","Wooden rod",StatKind::Rod,0,0,0,1,100,0,UpgradeId::Count},
    {UpgradeId::Rod3,"rod_3","Silver reel",StatKind::Rod,0,0,1,1,225,0,UpgradeId::Rod2},
    {UpgradeId::Rod4,"rod_4","Copper rod",StatKind::Rod,0,0,2,1,450,0,UpgradeId::Rod3},
    {UpgradeId::Rod5,"rod_5","Master rod",StatKind::Rod,0,0,3,1,800,0,UpgradeId::Rod4},
    {UpgradeId::Box2,"box_2","Room to grow",StatKind::Box,0,1,0,1,75,0,UpgradeId::Count},
    {UpgradeId::Box3,"box_3","Sturdy storage",StatKind::Box,0,1,1,1,175,0,UpgradeId::Box2},
    {UpgradeId::Box4,"box_4","Deep fish box",StatKind::Box,0,1,2,1,350,0,UpgradeId::Box3},
    {UpgradeId::Box5,"box_5","Master fish box",StatKind::Box,0,1,3,1,600,0,UpgradeId::Box4},
    {UpgradeId::Wait1,"wait_1","Patient rhythm I",StatKind::Wait,1,2,0,3,150,.02f,UpgradeId::Rod2},
    {UpgradeId::Wait2,"wait_2","Patient rhythm II",StatKind::Wait,1,2,1,3,650,.02f,UpgradeId::Wait1},
    {UpgradeId::Wait3,"wait_3","Patient rhythm III",StatKind::Wait,1,2,2,3,2000,.02f,UpgradeId::Wait2},
    {UpgradeId::Reel1,"reel_1","Quick reel I",StatKind::Reel,1,3,0,3,100,.05f,UpgradeId::Rod2},
    {UpgradeId::Reel2,"reel_2","Quick reel II",StatKind::Reel,1,3,1,3,450,.05f,UpgradeId::Reel1},
    {UpgradeId::Reel3,"reel_3","Quick reel III",StatKind::Reel,1,3,2,3,1500,.05f,UpgradeId::Reel2},
    {UpgradeId::Accuracy1,"accuracy_1","Steady hands I",StatKind::Accuracy,2,4,0,3,150,.02f,UpgradeId::Rod2},
    {UpgradeId::Accuracy2,"accuracy_2","Steady hands II",StatKind::Accuracy,2,4,1,3,300,.02f,UpgradeId::Accuracy1},
    {UpgradeId::Accuracy3,"accuracy_3","Steady hands III",StatKind::Accuracy,2,4,2,3,600,.03f,UpgradeId::Accuracy2},
    {UpgradeId::Accuracy4,"accuracy_4","Steady hands IV",StatKind::Accuracy,2,4,3,3,1000,.03f,UpgradeId::Accuracy3},
    {UpgradeId::Accuracy5,"accuracy_5","Steady hands V",StatKind::Accuracy,2,4,4,3,1600,.03f,UpgradeId::Accuracy4},
    {UpgradeId::Accuracy6,"accuracy_6","Steady hands VI",StatKind::Accuracy,2,4,5,3,2400,.03f,UpgradeId::Accuracy5},
    {UpgradeId::Rare1,"rare_1","Lucky waters I",StatKind::Rare,3,5,0,3,150,.05f,UpgradeId::Rod3},
    {UpgradeId::Rare2,"rare_2","Lucky waters II",StatKind::Rare,3,5,1,3,450,.05f,UpgradeId::Rare1},
    {UpgradeId::Rare3,"rare_3","Lucky waters III",StatKind::Rare,3,5,2,3,1100,.05f,UpgradeId::Rare2},
    {UpgradeId::Rare4,"rare_4","Lucky waters IV",StatKind::Rare,3,5,3,3,2400,.05f,UpgradeId::Rare3},
    {UpgradeId::Sale1,"sale_1","Good business I",StatKind::Sale,3,6,0,3,125,.025f,UpgradeId::Box2},
    {UpgradeId::Sale2,"sale_2","Good business II",StatKind::Sale,3,6,1,3,350,.025f,UpgradeId::Sale1},
    {UpgradeId::Sale3,"sale_3","Good business III",StatKind::Sale,3,6,2,3,900,.025f,UpgradeId::Sale2},
    {UpgradeId::Sale4,"sale_4","Good business IV",StatKind::Sale,3,6,3,3,1800,.025f,UpgradeId::Sale3}
}};
inline const UpgradeDefinition* upgradeDefinition(UpgradeId id) {
    const auto index = static_cast<std::size_t>(id);
    return index < kUpgradeCount ? &kUpgrades[index] : nullptr;
}
struct StatsEffects {
    float waitReduction = 0, reelReduction = 0, failureReduction = 0, rareBonus = 0, saleBonus = 0;
    float successChance(float base) const {
        return 1.0f - (1.0f - std::clamp(base, 0.0f, 1.0f)) * (1.0f - std::clamp(failureReduction, 0.0f, .48f));
    }
};
