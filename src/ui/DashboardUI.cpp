#include "UI.h"
#include "AssetTexture.h"
#include <algorithm>
#include <array>

namespace {
constexpr Color kBg{15,26,35,255}, kInk{231,239,233,255}, kMuted{145,170,176,255};
constexpr Color kMint{135,216,189,255}, kGold{239,200,123,255}, kSpecial{233,160,188,255};
constexpr Color kBanner{92,36,42,255};
struct PaperSlot {
    const char* name;
    const char* hint;
    int col = 0;
    int row = 0;
    UpgradeId upgrade = UpgradeId::Count;
};
constexpr std::array<PaperSlot, 4> kPaper{{
    {"Bait", "Lucky waters — rarer fish.", 0, 0, UpgradeId::Rare1},
    {"Jacket", "Good business — better sale prices.", 0, 1, UpgradeId::Sale1},
    {"Line", "Patient rhythm — shorter waits.", 1, 0, UpgradeId::Wait1},
    {"Reel", "Quick reel — faster retrieve.", 1, 1, UpgradeId::Reel1},
}};
Rectangle paperRect(float x, const PaperSlot& slot) {
    return {x + (slot.col ? 336.0f : 144.0f), 40.0f + slot.row * 36.0f, 32, 32};
}
void drawGearIcon(const PaperSlot& slot, Rectangle dest, Color ink) {
    const float s = 2;
    const Vector2 p{dest.x + 8, dest.y + 8};
    auto px = [&](int x, int y, int w = 1, int h = 1) {
        DrawRectangleRec({p.x + x * s, p.y + y * s, w * s, h * s}, ink);
    };
    if (slot.upgrade == UpgradeId::Rare1) {
        px(3, 0, 2, 2); px(1, 2, 6, 2); px(2, 4, 4, 2); px(3, 6, 2, 1);
    } else if (slot.upgrade == UpgradeId::Sale1) {
        px(2, 1, 4, 1); px(1, 2, 1, 4); px(6, 2, 1, 4); px(2, 6, 4, 1); px(3, 2, 2, 4);
    } else if (slot.upgrade == UpgradeId::Wait1) {
        px(2, 0, 4, 1); px(0, 2, 1, 4); px(7, 2, 1, 4); px(2, 7, 4, 1); px(3, 2, 2, 3);
    } else {
        px(1, 1, 6, 1); px(1, 6, 6, 1); px(1, 2, 1, 4); px(6, 2, 1, 4); px(3, 3, 2, 2);
    }
}
}

UIAction UI::dashboard(const FishingSystem& fishing, const Player& player,
                       const std::optional<FishInstance>& lastCatch, bool autoSellCommon) {
    UIAction action;
    const bool craftVisible = craftOpen_, statsVisible = summaryOpen_;
    const float x = static_cast<float>(dashboardX());
    const auto r = [&](float left,float top,float w,float h) { return Rectangle{x+left,top,w,h}; };
    ClearBackground(kBg);
    BeginMode2D(camera());
    card(r(0,0,512,352));
    text(TextFormat("$%lld", static_cast<long long>(player.money())), x+16, 10, 16, kGold);
    DrawRectangleRounded(r(166,6,180,26), 0.3f, 6, kBanner);
    DrawRectangleLinesEx(r(166,6,180,26), 1, kGold);
    fitText(player.name().empty() ? "ANGLER" : player.name(), r(174,6,164,26), 16, kGold);
    if (!portrait_.id) {
        portrait_ = loadAssetTexture("assets/player/shoulder_portrait.png");
        if (portrait_.id) SetTextureFilter(portrait_,TEXTURE_FILTER_POINT);
    }
    card(r(184,36,144,144),kBg);
    if (portrait_.id) DrawTexturePro(portrait_,{0,0,static_cast<float>(portrait_.width),static_cast<float>(portrait_.height)},r(184,36,144,144),{0,0},0,WHITE);

    for (const auto& slot : kPaper) {
        const auto dest = paperRect(x, slot);
        const bool filled = player.upgradeRank(slot.upgrade) > 0;
        skin_.draw(UISkin::Slot, filled ? 4 : 0, dest);
        if (filled) drawGearIcon(slot, dest, kGold);
        if (CheckCollisionPointRec(input_.mouse, dest)) {
            tooltip_ = TextFormat("%s — %s", slot.name, slot.hint);
            DrawRectangleLinesEx(dest, 1, kMint);
        }
        if (clicked(dest)) {
            craftOpen_ = true;
            craftSelection_ = slot.upgrade;
        }
    }

    const auto& box = player.fishBox();
    if (selected_ && *selected_ >= box.size()) selected_.reset();

    if (button(r(16,188,160,24),"Inventory",true,!collectionOpen_,0,true)) collectionOpen_=false;
    if (button(r(184,188,160,24),"Collection",true,collectionOpen_,1,true)) collectionOpen_=true;
    if (button(r(352,188,144,24),"Sell all",player.boxValue(true)>0 && !collectionOpen_,false,4)) action.type=UIActionType::SellAll;
    if (CheckCollisionPointRec(input_.mouse,r(352,188,144,24))) tooltip_="Sell all unlocked fish. Protected fish stay in your box.";

    if (collectionOpen_) {
        card(r(16,218,480,92),kBg);
        int discovered = 0;
        const int total = static_cast<int>(fishing.database().all().size());
        for (int id = 0; id < total; ++id) if (player.progress(id).discovered) ++discovered;
        text(TextFormat("%d / %d discovered", discovered, total), x+24, 222, 11, kMint);
        constexpr int visible = 4;
        if (CheckCollisionPointRec(input_.mouse, r(16,218,480,92)) && input_.wheel != 0)
            collectionScroll_ -= input_.wheel > 0 ? 1 : -1;
        collectionScroll_ = std::clamp(collectionScroll_, 0, std::max(0, total - visible));
        for (int id = collectionScroll_; id < std::min(total, collectionScroll_ + visible); ++id) {
            const auto entry = player.progress(id);
            const auto& species = fishing.database().at(id);
            const float y = 240 + static_cast<float>(id - collectionScroll_) * 16;
            if (entry.discovered) fishIcon({id, entry.recordWeight, 0}, species, r(22, y - 4, 22, 18));
            else skin_.draw(UISkin::Unknown, 0, r(24, y - 2, 14, 14));
            text(species.name.c_str(), x+48, y, 12, entry.discovered ? kInk : kMuted);
            text(entry.discovered ? TextFormat("%.2f kg", entry.recordWeight) : "--", x+400, y, 12, entry.discovered ? kMint : kMuted);
        }
    } else {
        const int rows = (player.capacity() + kGridCols - 1) / kGridCols;
        const int maxScroll = std::max(0, rows - kVisibleRows);
        if (CheckCollisionPointRec(input_.mouse, r(12,214,348,106)) && input_.wheel != 0)
            inventoryScroll_ -= input_.wheel > 0 ? 1 : -1;
        inventoryScroll_ = std::clamp(inventoryScroll_, 0, maxScroll);
        const int first = inventoryScroll_ * kGridCols;
        const int visible = kGridCols * kVisibleRows;
        for (int i = 0; i < visible; ++i) {
            const int slot = first + i;
            if (slot >= kGridSlots) break;
            auto dest = slotRect(slot, inventoryScroll_); dest.x += x;
            const bool filled = static_cast<std::size_t>(slot) < box.size(), available = slot < player.capacity();
            const auto rarity = filled ? fishing.database().at(box[static_cast<std::size_t>(slot)].speciesId).rarity : FishRarity::Common;
            skin_.draw(UISkin::Slot, !available ? 1 : !filled ? 0 : rarity == FishRarity::Common ? 2 : rarity == FishRarity::Uncommon ? 3 : 4, dest);
            if (filled) {
                if (rarity == FishRarity::Special) DrawRectangleLinesEx(dest, 1, kSpecial);
                fishIcon(box[static_cast<std::size_t>(slot)], fishing.database().at(box[static_cast<std::size_t>(slot)].speciesId), dest);
                const bool hover = CheckCollisionPointRec(input_.mouse, dest);
                if (hover) { hovered_ = static_cast<std::size_t>(slot); skin_.draw(UISkin::Overlay, 0, dest); }
                if (selected_ && *selected_ == static_cast<std::size_t>(slot)) skin_.draw(UISkin::Overlay, 1, dest);
                if (box[static_cast<std::size_t>(slot)].locked) skin_.draw(UISkin::Overlay, 2, dest);
                if (hover && input_.rightPressed && !box[static_cast<std::size_t>(slot)].locked)
                    action = {UIActionType::SellStored, static_cast<std::size_t>(slot)};
                if (clicked(dest)) {
                    if (input_.altDown) action = {UIActionType::ToggleLock, static_cast<std::size_t>(slot)};
                    else selected_ = static_cast<std::size_t>(slot);
                }
            } else if (CheckCollisionPointRec(input_.mouse, dest))
                tooltip_ = available ? "Empty slot" : "Upgrade your Fish Box to unlock more slots. Scroll to see extra rows.";
        }
        if (maxScroll > 0) {
            DrawRectangle(static_cast<int>(x + 354), 218, 4, 96, Color{31,51,61,255});
            const float thumb = 96.0f * kVisibleRows / rows;
            DrawRectangleRec({x + 354, 218 + (96 - thumb) * inventoryScroll_ / maxScroll, 4, thumb}, kMint);
        }

        card(r(366,218,130,92),kBg);
        text(TextFormat("%d / %d", static_cast<int>(box.size()), player.capacity()), x+372, 220, 11, player.boxFull() ? Color{242,154,129,255} : kMuted);
        if (selected_) {
            const auto& fish=box[*selected_];
            const auto& species=fishing.database().at(fish.speciesId);
            if (!sprites_.drawIcon(species.name,r(408,222,40,40),2)) fishIcon(fish,species,r(408,222,40,40));
            fitText(species.name,r(372,262,118,14),11,kInk);
            if(button(r(372,278,58,22),"Sell",!fish.locked))action={UIActionType::SellStored,*selected_};
            if(button(r(434,278,54,22),"",true,fish.locked,fish.locked?6:5))action={UIActionType::ToggleLock,*selected_};
            if(CheckCollisionPointRec(input_.mouse,r(434,278,54,22)))tooltip_=fish.locked?"Unlock catch":"Protect catch";
        } else {
            skin_.draw(UISkin::Unknown,0,r(418,236,24,24));
            fitText("Select a catch",r(372,268,118,18),11,kMuted);
        }
    }

    if(button(r(16,318,172,24),"Craft",true,craftOpen_))craftOpen_=!craftOpen_;
    const auto money=TextFormat("$%lld",static_cast<long long>(player.money()));
    fitText(money,r(200,320,112,20),13,kGold);
    if(button(r(324,318,172,24),"Stats",true,summaryOpen_))summaryOpen_=!summaryOpen_;
    EndMode2D();

    BeginMode2D(camera({x,360}));
    scene(fishing,player,autoSellCommon);
    skin_.draw(UISkin::Status,0,{8,148,496,24});
    if(lastCatch) {
        fishIcon(*lastCatch,fishing.database().at(lastCatch->speciesId),{12,148,26,24});
        fitText(TextFormat("Last catch / %s / %.2f kg",fishing.database().at(lastCatch->speciesId).name.c_str(),lastCatch->weight),{44,150,450,20},12,kInk);
    } else text("Waiting for your next catch",16,154,12,kMuted);
    EndMode2D();
    BeginMode2D(camera());
    if(button(r(452,364,24,24),"",true,true,10))closePanels();
    if(button(r(480,364,24,24),"",true,false,8))toggleSettings();
    if(craftVisible) { const auto result=equipmentPanel(player,0); if(result.type!=UIActionType::None)action=result; }
    if(statsVisible) { const auto result=summaryPanel(fishing,player,x+520); if(result.type!=UIActionType::None)action=result; }
    if(!tooltip_.empty() && preferences_.showTooltips) { card(r(8,508,496,24),kBg); fitText(tooltip_,r(16,510,480,20),12,kInk); }
    if(hovered_ && preferences_.showTooltips && *hovered_<box.size())itemTooltip(box[*hovered_],fishing.database().at(box[*hovered_].speciesId),player);
    EndMode2D();
    if(input_.released)pressedButton_.reset();
    return action;
}

UIAction UI::equipmentPanel(const Player& player,float x) {
    UIAction action;
    card({x,0,288,kDashboardHeight});
    text("CRAFTING",x+16,16,21,kInk);
    text("EQUIPMENT",x+16,42,11,kMuted);
    if(button({x+252,12,24,24},"X"))craftOpen_=false;
    DrawLine(static_cast<int>(x+16),62,static_cast<int>(x+272),62,kMuted);
    for(int i=0;i<8;++i) {
        const auto& def=kUpgrades[i];
        const Rectangle slot{x+24+(i%4)*64.0f,82+(i/4)*72.0f,56,56};
        const bool maxed=player.upgradeRank(def.id)==def.maxRank;
        if(button(slot,TextFormat("%s %d",i<4?"Rod":"Box",def.stage+2),true,craftSelection_==def.id))craftSelection_=def.id;
        if(maxed)DrawRectangleRec({slot.x+slot.width-7,slot.y+4,3,3},kMint);
    }
    const auto& def=*upgradeDefinition(craftSelection_);
    const bool maxed=player.upgradeRank(def.id)==def.maxRank;
    if(button({x+44,300,200,28},maxed?"Completed":"Craft",player.canUpgrade(def.id),false,7))action={UIActionType::UpgradeStat,static_cast<std::size_t>(def.id)};
    DrawLine(static_cast<int>(x+16),354,static_cast<int>(x+272),354,kMuted);
    text("CRAFT RESULT",x+28,374,11,kMuted);
    fitText(def.name,{x+28,400,232,26},18,kInk);
    text(maxed?"Owned":TextFormat("Cost: $%d",player.upgradeCost(def.id)),x+28,439,14,maxed?kMint:kGold);
    if(!maxed && !player.upgradeUnlocked(def.id))text("Previous tier required",x+28,465,12,kMuted);
    return action;
}

UIAction UI::summaryPanel(const FishingSystem& fishing,const Player& player,float x) {
    UIAction action;
    card({x,0,288,kDashboardHeight});
    text("STATS",x+16,16,21,kInk);
    text("ANGLER PROGRESSION",x+16,42,11,kMuted);
    if(button({x+252,12,24,24},"X"))summaryOpen_=false;
    const auto effects=player.stats();
    text("CATCH CHANCE",x+30,88,12,kMuted);
    text(TextFormat("%.1f%%",effects.successChance(FishingSystem::successChance(fishing.localHour()))*100),x+204,88,14,kMint);
    text("REEL TIME",x+30,134,12,kMuted);
    text(TextFormat("%.2fs",1-effects.reelReduction),x+204,134,14,kInk);
    text("RARE BONUS",x+30,180,12,kMuted);
    text(TextFormat("+%.0f%%",effects.rareBonus*100),x+204,180,14,kGold);
    DrawLine(static_cast<int>(x+16),226,static_cast<int>(x+272),226,kMuted);
    text("FISH BOX",x+30,258,12,kInk);
    text(TextFormat("%d slots",player.capacity()),x+202,258,12,kMint);
    const bool boxMax=player.boxLevel()==5, rodMax=player.rodLevel()==5;
    text(boxMax?"Maximum capacity":TextFormat("Next tier $%d",player.boxUpgradeCost()),x+30,290,12,kMuted);
    if(button({x+30,316,228,26},"Increase capacity",!boxMax && player.money()>=player.boxUpgradeCost()))action.type=UIActionType::UpgradeBox;
    text("FISHING ROD",x+30,389,12,kInk);
    text(TextFormat("Level %d",player.rodLevel()),x+30,418,12,kMint);
    text(rodMax?"MAX":TextFormat("$%d",player.rodUpgradeCost()),x+204,418,12,kGold);
    if(button({x+30,448,228,26},"Upgrade rod",!rodMax && player.money()>=player.rodUpgradeCost()))action.type=UIActionType::UpgradeRod;
    if(button({x+30,490,228,26},"Progression tree",true,false,7))toggleStats();
    return action;
}
