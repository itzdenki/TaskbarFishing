#include "UI.h"
#include <algorithm>
#include <cmath>
#include <string>

namespace {
constexpr Rectangle kTree{16,146,624,410};
constexpr Color kBg{15,26,35,255}, kLine{48,70,79,255};
constexpr Color kInk{231,239,233,255}, kMuted{145,170,176,255}, kGold{239,200,123,255}, kMint{135,216,189,255};
constexpr Color kBranch[]{{239,200,123,255},{112,191,242,255},{237,139,117,255},{183,153,235,255}};
// Tiny code-native pixel icons share silhouettes across each upgrade chain.
void statIcon(StatKind kind, Vector2 p, float scale, Color ink) {
    auto pixel = [&](int x,int y,int w=1,int h=1) { DrawRectangleRec({p.x+x*scale,p.y+y*scale,w*scale,h*scale},ink); };
    switch(kind) {
    case StatKind::Rod:
        for(int i=0;i<10;++i) pixel(3+i,13-i,2,1);
        pixel(11,5,1,8); pixel(9,13,3,1); pixel(4,10,3,3); break;
    case StatKind::Box:
        pixel(2,5,12,2); pixel(2,7,2,7); pixel(12,7,2,7); pixel(4,12,8,2); pixel(7,7,2,3); pixel(6,3,4,2); break;
    case StatKind::Wait:
        pixel(4,2,8,2); pixel(4,12,8,2); pixel(2,4,2,8); pixel(12,4,2,8); pixel(7,5,2,4); pixel(9,8,3,2); break;
    case StatKind::Reel:
        pixel(3,3,10,2); pixel(3,11,10,2); pixel(2,5,2,6); pixel(12,5,2,6); pixel(6,6,4,4); pixel(0,7,2,2); pixel(14,8,2,2); break;
    case StatKind::Accuracy:
        pixel(6,1,4,2); pixel(6,13,4,2); pixel(1,6,2,4); pixel(13,6,2,4);
        pixel(4,4,8,1); pixel(4,11,8,1); pixel(4,5,1,6); pixel(11,5,1,6); pixel(7,7,2,2); break;
    case StatKind::Rare:
        pixel(6,1,4,3); pixel(3,4,10,2); pixel(2,6,12,3); pixel(4,9,8,2); pixel(6,11,4,2); pixel(7,13,2,2); break;
    case StatKind::Sale:
        pixel(4,2,8,2); pixel(4,12,8,2); pixel(2,4,2,8); pixel(12,4,2,8); pixel(7,4,2,8); pixel(5,5,6,2); pixel(5,9,6,2); break;
    }
}
const char* description(StatKind kind) {
    switch(kind) {
    case StatKind::Rod:return "A faster rod with better odds of finding rare fish.";
    case StatKind::Box:return "More space to keep fishing before you need to sell.";
    case StatKind::Wait:return "Spend less time waiting for the next bite.";
    case StatKind::Reel:return "Bring each hooked fish to the pier more quickly.";
    case StatKind::Accuracy:return "Reduce the failed part of your hourly catch chance.";
    case StatKind::Rare:return "Increase rare-fish weight in the catch pool.";
    case StatKind::Sale:return "Earn more whenever you sell, including auto sales.";
    }
    return "";
}
}

Rectangle UI::statsNodeRect(UpgradeId id) const {
    const auto* def = upgradeDefinition(id);
    if (!def) return {};
    return {kTree.x + treePan_.x + (92 + def->stage * 86)*treeZoom_,
            kTree.y + treePan_.y + (18 + def->lane * 54)*treeZoom_,40*treeZoom_,40*treeZoom_};
}

UIAction UI::statsPanel(const FishingSystem& fishing, const Player& player) {
    UIAction result;
    const bool inTree = CheckCollisionPointRec(input_.mouse,kTree);
    if (inTree && input_.wheel != 0) {
        const float old = treeZoom_;
        treeZoom_ = std::clamp(old + input_.wheel*.1f,.65f,1.5f);
        treePan_.x = input_.mouse.x-kTree.x-(input_.mouse.x-kTree.x-treePan_.x)*treeZoom_/old;
        treePan_.y = input_.mouse.y-kTree.y-(input_.mouse.y-kTree.y-treePan_.y)*treeZoom_/old;
    }
    if (inTree && input_.pressed) { treePress_ = input_.mouse; treePanAtPress_ = treePan_; treeDragging_ = false; }
    if (treePress_ && (input_.down || input_.released)) {
        const Vector2 delta{input_.mouse.x-treePress_->x,input_.mouse.y-treePress_->y};
        if (delta.x*delta.x+delta.y*delta.y > 16) treeDragging_ = true;
        if (treeDragging_) treePan_ = {treePanAtPress_.x+delta.x,treePanAtPress_.y+delta.y};
        if (input_.released) {
            if (!treeDragging_ && inTree) for(const auto& def:kUpgrades)
                if(CheckCollisionPointRec(input_.mouse,statsNodeRect(def.id))) selectedUpgrade_=def.id;
            treePress_.reset();
        }
    }
    treePan_.x=std::clamp(treePan_.x,std::min(0.f,kTree.width-624*treeZoom_)-16,16.f);
    treePan_.y=std::clamp(treePan_.y,std::min(0.f,kTree.height-410*treeZoom_)-16,16.f);
    const auto effects=player.stats();
    const float base=FishingSystem::successChance(localTime_.hour), finalChance=effects.successChance(base);
    ClearBackground(kBg);
    text("STATS",22,17,26,kInk);
    text("A little better with every cast.",122,24,14,kMuted);
    const std::string money=TextFormat("$%lld",static_cast<long long>(player.money()));
    text(money.c_str(),772-textWidth(money.c_str(),24),17,24,kGold);
    if(button({794,14,150,32},"Back to lake",true,false,10)) backFromStats();

    auto metric=[&](float x,const char* label,const std::string& value,Color color){
        card({x,66,176,65}); text(label,x+12,76,11,kMuted); text(value.c_str(),x+12,98,17,color);
    };
    metric(16,"CATCH CHANCE",TextFormat("%.1f%% -> %.1f%%",base*100,finalChance*100),kMint);
    metric(204,"AVERAGE WAIT",TextFormat("%.2f seconds",8.5f*FishingSystem::waitMultiplier(player.rodLevel())*(1-effects.waitReduction)),kBranch[1]);
    metric(392,"REEL TIME",TextFormat("%.2f seconds",1-effects.reelReduction),kBranch[1]);
    metric(580,"RARE WEIGHT",TextFormat("+%.0f%% from stats",effects.rareBonus*100),kBranch[3]);
    metric(768,"SALE BONUS",TextFormat("+%.1f%%",effects.saleBonus*100),kGold);
    card(kTree);
    // Scissor coordinates are window pixels, unlike the camera-scaled drawing inside.
    const float s=scale();
    BeginScissorMode(static_cast<int>((kTree.x+1)*s),static_cast<int>((kTree.y+1)*s),static_cast<int>(std::ceil((kTree.width-2)*s)),static_cast<int>(std::ceil((kTree.height-2)*s)));
    for(int x=0;x<624;x+=18) for(int y=0;y<410;y+=18)
        DrawPixel(static_cast<int>(kTree.x+x),static_cast<int>(kTree.y+y),{31,48,57,255});
    for(const auto& def:kUpgrades) {
        const auto* parent=upgradeDefinition(def.prerequisite);
        if(!parent)continue;
        const auto a=statsNodeRect(parent->id), b=statsNodeRect(def.id);
        const Color color=player.upgradeUnlocked(def.id)?Fade(kBranch[def.branch],.75f):kLine;
        if(parent->lane==def.lane) DrawLineEx({a.x+a.width,a.y+a.height/2},{b.x,b.y+b.height/2},2,color);
        else {
            const float gutter=kTree.x+treePan_.x+68*treeZoom_;
            DrawLineEx({a.x,a.y+a.height/2},{gutter,a.y+a.height/2},1,color);
            DrawLineEx({gutter,a.y+a.height/2},{gutter,b.y+b.height/2},1,color);
            DrawLineEx({gutter,b.y+b.height/2},{b.x,b.y+b.height/2},1,color);
        }
    }
    constexpr const char* lanes[]{"ROD","BOX","WAIT","REEL","AIM","RARE","SALE"};
    constexpr int branches[]{0,0,1,1,2,3,3};
    for(int lane=0;lane<7;++lane)
        text(lanes[lane],kTree.x+treePan_.x+10*treeZoom_,kTree.y+treePan_.y+(32+lane*54)*treeZoom_,10*treeZoom_,kBranch[branches[lane]]);
    for(const auto& def:kUpgrades) {
        const auto rect=statsNodeRect(def.id);
        const int rank=player.upgradeRank(def.id);
        const bool unlocked=player.upgradeUnlocked(def.id), maxed=rank==def.maxRank;
        const bool hover=inTree&&CheckCollisionPointRec(input_.mouse,rect);
        const Color accent=kBranch[def.branch];
        DrawRectangleRec(rect,rank?Fade(accent,.18f):Color{17,29,38,255});
        DrawRectangleLinesEx(rect,(def.id==selectedUpgrade_?3:1)*treeZoom_,def.id==selectedUpgrade_?kInk:!unlocked?kLine:accent);
        if(hover) DrawRectangleLinesEx({rect.x-2,rect.y-2,rect.width+4,rect.height+4},1,kInk);
        statIcon(def.kind,{rect.x+8*treeZoom_,rect.y+5*treeZoom_},1.5f*treeZoom_,unlocked?accent:Color{71,88,96,255});
        DrawRectangleRec({rect.x,rect.y+29*treeZoom_,rect.width,11*treeZoom_},maxed?Fade(accent,.65f):kBg);
        text(TextFormat("%d/%d",rank,def.maxRank),rect.x+11*treeZoom_,rect.y+29*treeZoom_,10*treeZoom_,maxed?kBg:kInk);
        if(player.canUpgrade(def.id)) DrawRectangleRec({rect.x+34*treeZoom_,rect.y+2*treeZoom_,4*treeZoom_,4*treeZoom_},kMint);
        else if(unlocked&&!maxed) DrawCircle(static_cast<int>(rect.x+36*treeZoom_),static_cast<int>(rect.y+4*treeZoom_),2*treeZoom_,kGold);
    }
    EndScissorMode();
    card({656,146,288,410});
    const auto& def=*upgradeDefinition(selectedUpgrade_);
    const int rank=player.upgradeRank(def.id);
    const bool maxed=rank==def.maxRank, unlocked=player.upgradeUnlocked(def.id);
    const auto next=player.previewStats(def.id);
    const Color accent=kBranch[def.branch];
    statIcon(def.kind,{674,163},2,accent);
    text(TextFormat("RANK %d / %d",rank,def.maxRank),722,163,12,accent);
    text(maxed?"MASTERED":unlocked?"AVAILABLE":"LOCKED",722,183,12,maxed?kMint:unlocked?kInk:kMuted);
    fitText(def.name,{674,209,250,25},20,kInk);
    // Wrap readable descriptive text rather than ellipsizing the requirement explanation.
    auto wrapped=[&](std::string words,float y,Color color){
        std::string line;
        while(!words.empty()) {
            const auto split=words.find(' ');
            const std::string word=words.substr(0,split);
            if(!line.empty()&&textWidth((line+" "+word).c_str(),13)>250) {text(line.c_str(),674,y,13,color);y+=19;line.clear();}
            if(!line.empty())line+=' ';
            line+=word;
            if(split==std::string::npos)break;
            words.erase(0,split+1);
        }
        if(!line.empty())text(line.c_str(),674,y,13,color);
    };
    wrapped(description(def.kind),246,kMuted);
    text(maxed?"CURRENT EFFECT":"CURRENT  ->  NEXT",674,307,11,accent);
    std::string effect, secondary;
    const int nextRank=maxed?rank:rank+1;
    switch(def.kind) {
    case StatKind::Rod: {
        const int level=maxed?player.rodLevel():def.stage+2;
        effect=TextFormat("Rod Lv.%d -> Lv.%d",player.rodLevel(),level);
        secondary=TextFormat("Average wait %.2fs -> %.2fs",8.5f*FishingSystem::waitMultiplier(player.rodLevel())*(1-effects.waitReduction),8.5f*FishingSystem::waitMultiplier(level)*(1-effects.waitReduction));break;
    }
    case StatKind::Box: {
        effect=TextFormat("Capacity %d -> %d fish",player.capacity(),maxed?player.capacity():kBoxCapacity[static_cast<std::size_t>(def.stage+1)]);break;
    }
    case StatKind::Wait:effect=TextFormat("Wait reduction %.0f%% -> %.0f%%",effects.waitReduction*100,next.waitReduction*100);break;
    case StatKind::Reel:effect=TextFormat("Reel time %.2fs -> %.2fs",1-effects.reelReduction,1-next.reelReduction);break;
    case StatKind::Accuracy:
        effect=TextFormat("Failure reduced %.0f%% -> %.0f%%",effects.failureReduction*100,next.failureReduction*100);
        secondary=TextFormat("Catch now %.1f%% -> %.1f%%",finalChance*100,next.successChance(base)*100);break;
    case StatKind::Rare:effect=TextFormat("Rare weight +%.0f%% -> +%.0f%%",effects.rareBonus*100,next.rareBonus*100);break;
    case StatKind::Sale:effect=TextFormat("Sale bonus +%.1f%% -> +%.1f%%",effects.saleBonus*100,next.saleBonus*100);break;
    }
    fitText(effect,{674,331,250,20},14,kInk);
    fitText(secondary,{674,355,250,20},12,kMuted);
    const auto* prerequisite=upgradeDefinition(def.prerequisite);
    wrapped(prerequisite?std::string("Requires: ")+prerequisite->name+" (max rank)":"No prerequisite. Start here.",390,unlocked?kMint:kBranch[2]);
    text(maxed?"Permanent upgrade completed.":TextFormat("Next rank %d: $%d",nextRank,player.upgradeCost(def.id)),674,457,15,maxed?kMint:kGold);
    const std::string label=maxed?"Max rank":!unlocked?"Complete prerequisite":!player.canUpgrade(def.id)?"Not enough money":"Upgrade";
    if(button({674,494,252,40},label,player.canUpgrade(def.id),false,7)) result={UIActionType::UpgradeStat,static_cast<std::size_t>(def.id)};

    const bool blocked=fishing.catchSettled()&&player.boxFull();
    skin_.draw(UISkin::Status,0,{16,571,928,38});
    text(blocked?"Fish Box full - return to the lake to sell fish.":"Permanent upgrades. Every branch can be completed.",28,582,13,blocked?kBranch[2]:kMuted);
    text(TextFormat("Fish Box %d/%d",static_cast<int>(player.fishBox().size()),player.capacity()),774,582,13,kInk);
    text("Drag tree to pan   /   Wheel to zoom   /   Click a node to inspect",18,620,11,kMuted);
    text(localTime_.label().c_str(),754,620,11,kMuted);
    if(input_.released)pressedButton_.reset();
    return result;
}
