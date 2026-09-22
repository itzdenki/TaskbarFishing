#include "player/Player.h"
#include "fishing/FishingSystem.h"
#include "save/SaveSystem.h"
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <set>
#include <stdexcept>

void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
void near(float a,float b,const char* message){check(std::abs(a-b)<.0001f,message);}
Player funded(){Player p;for(int i=0;i<5000;++i)p.sell({4,4,240});return p;}
Player maxed(){auto p=funded();for(const auto& def:kUpgrades)while(p.tryUpgrade(def.id)){}return p;}
void testTree(){
    Player p;
    check(!p.tryUpgrade(UpgradeId::Rod2)&&p.money()==0,"insufficient funds are atomic");
    p=funded();
    check(!p.tryUpgrade(UpgradeId::Accuracy1)&&!p.tryUpgrade(UpgradeId::Rod3),"locked nodes cannot bypass prerequisites");
    check(!p.tryUpgrade(static_cast<UpgradeId>(999)),"unknown upgrade rejected");
    const auto before=p.money();
    std::set<std::string> ids;
    int purchases=0;
    for(std::size_t i=0;i<kUpgradeCount;++i){
        const auto& def=kUpgrades[i];
        check(static_cast<std::size_t>(def.id)==i&&ids.insert(def.key).second,"stable unique IDs");
        const auto rankBefore=p.upgradeRank(def.id);
        p.previewStats(def.id);
        check(p.upgradeRank(def.id)==rankBefore,"preview never purchases");
        for(int rank=0;rank<def.maxRank;++rank){
            check(p.upgradeCost(def.id)==def.baseCost*(1<<rank),"1x 2x 4x prices");
            const auto money=p.money();
            const auto cost=p.upgradeCost(def.id);
            check(p.tryUpgrade(def.id),"legal upgrade succeeds");
            check(p.money()==money-cost&&p.upgradeRank(def.id)==rank+1,"one click buys one rank");
            ++purchases;
        }
        const auto money=p.money();
        check(!p.tryUpgrade(def.id)&&p.money()==money,"maxed upgrades charge nothing");
    }
    check(purchases==68&&before-p.money()==130000,"28 nodes, 68 purchases, exact total cost");
    check(p.validUpgrades()&&p.rodLevel()==5&&p.boxLevel()==5&&p.capacity()==70,"complete legal tree");
    const auto stats=p.stats();
    near(stats.waitReduction,.18f,"18 percent shorter waits");
    near(stats.reelReduction,.45f,"45 percent shorter reeling");
    near(stats.failureReduction,.48f,"48 percent fewer failures");
    near(stats.rareBonus,.60f,"60 percent rare weight bonus");
    near(stats.saleBonus,.30f,"30 percent sale bonus");
    near(stats.successChance(.28f),.6256f,"13h final odds");
    near(stats.successChance(.88f),.9376f,"18h final odds");
}
void testGameplay(){
    const auto effects=maxed().stats();
    FishingSystem baseline(123),upgraded(123);
    upgraded.setStats(effects);baseline.start(5);upgraded.start(5);
    baseline.update(.65f,5);upgraded.update(.65f,5);
    near(upgraded.remaining(),baseline.remaining()*.82f,"waiting modifier reaches timer");
    const float activeWait=baseline.remaining();
    baseline.setStats(effects);
    near(baseline.remaining(),activeWait,"upgrade does not resize active wait");
    baseline.update(baseline.remaining(),5);baseline.update(.5f,5);
    near(baseline.remaining(),.55f,"next reeling uses reduction");
    baseline.setStats({});near(baseline.remaining(),.55f,"active reel remains unchanged");
    baseline.setStats(effects);baseline.setLocalHour(18);baseline.update(.55f,5);
    near(baseline.lastAttemptChance(),.9376f,"roll records upgraded probability");
    baseline.setStats({});baseline.setLocalHour(13);
    near(baseline.lastAttemptChance(),.9376f,"completed roll probability never changes");
    for(int hour=0;hour<24;++hour)for(int rod:{1,5}){
        FishingSystem trials(1000+hour);trials.setStats(effects);trials.setLocalHour(hour);trials.start(rod);
        int success=0,total=0;
        while(total<5000){const auto event=trials.update(trials.remaining(),rod);
            if(event==FishingEvent::Caught){++total;++success;trials.resolveCatch(rod);}
            else if(event==FishingEvent::Escaped)++total;
        }
        check(std::abs(success/5000.f-effects.successChance(FishingSystem::successChance(hour)))<.03f,"upgraded success frequencies match all hours and rods");
    }
    FishDatabase db;
    for(int rod:{1,5}){
        std::mt19937 rng(18);int rare=0;
        for(int i=0;i<60000;++i){auto fish=db.roll(rng,rod,effects.rareBonus);check(db.valid(fish),"bonuses preserve fish validation");if(fish.speciesId==4)++rare;}
        const double weight=3*(1+.12*(rod-1))*1.6;
        check(std::abs(rare/60000.0-weight/(97+weight))<.006,"rare weights combine with rod and normalize");
    }
}
void testSales(){
    auto p=funded();p.upgradeBox();
    for(auto id:{UpgradeId::Sale1,UpgradeId::Sale2})for(int i=0;i<3;++i)check(p.tryUpgrade(id),"sale chain");
    check(p.tryUpgrade(UpgradeId::Sale3),"seventh sale rank");
    const FishInstance fish{1,2.86f,20};
    check(p.salePrice(fish)==24,"exact half-up rounding at 17.5 percent");
    p.keep(fish);p.keep(fish);p.toggleLock(0);
    check(p.boxValue(false)==48&&p.boxValue(true)==24,"displayed box values use sale bonus");
    check(!p.sellStored(0),"sale bonus cannot bypass a lock");
    const auto before=p.money();check(p.sellAll()==24&&p.money()==before+24&&p.fishBox().size()==1,"sell-all and locks");
    p.toggleLock(0);check(p.sellStored(0)&&p.money()==before+48,"single sale matches quote");
    p.sell(fish);check(p.money()==before+72,"auto-sale path uses same price");
    check(fish.sellPrice==20&&FishDatabase{}.valid(fish),"base price never rewritten");
}
void testSave(){
    struct Temp {
        std::filesystem::path dir=std::filesystem::temp_directory_path()/ ("TaskbarFishing-stats-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        Temp(){std::filesystem::create_directories(dir);}
        ~Temp(){std::error_code error;std::filesystem::remove_all(dir,error);}
    }temp;
    FishDatabase db;std::string error;SaveData data,loaded;data.player=maxed();
    const FishInstance fish{1,3,21,true};data.player.recordCatch(fish);data.player.keep(fish);data.pendingCatch=fish;
    const auto file=temp.dir/"full.save";
    check(SaveSystem::save(file,data,db,error)&&SaveSystem::load(file,loaded,db,error)==LoadStatus::Loaded,"current save roundtrip");
    for(const auto& def:kUpgrades)check(loaded.player.upgradeRank(def.id)==def.maxRank,"every rank restored");
    check(loaded.player.money()==data.player.money()&&loaded.player.fishBox()[0].locked&&loaded.pendingCatch.has_value(),"inventory economy and pending fish retained");
    std::ifstream in(file);std::string source((std::istreambuf_iterator<char>(in)),{});in.close();
    auto corrupt=[&](const char* from,const char* to){auto text=source;const auto pos=text.find(from);check(pos!=std::string::npos,"fixture field exists");text.replace(pos,std::string(from).size(),to);{std::ofstream out(temp.dir/"bad.save");out<<text;}check(SaveSystem::load(temp.dir/"bad.save",loaded,db,error)==LoadStatus::Invalid,"invalid stat save rejected");};
    corrupt("accuracy_1 3","accuracy_1 4");corrupt("wait_1 3","wait_1 0");corrupt("sale_4 3","sale_4 -1");corrupt("stats 20","stats 19");corrupt("rare_1 3","unknown 3");
    for(int version:{1,2}){
        auto legacy=source;legacy.replace(legacy.find("TASKBAR_FISHING 6"),std::string("TASKBAR_FISHING 6").size(),"TASKBAR_FISHING "+std::to_string(version));
        const auto preferences=legacy.find("preferences ");legacy.erase(preferences,legacy.find('\n',preferences)-preferences+1);
        const auto name=legacy.find("name ");legacy.erase(name,legacy.find('\n',name)-name+1);
        const auto begin=legacy.find("stats ");legacy.erase(begin,legacy.find("settings ")-begin);
        if(version==1){const auto pos=legacy.find("1 3 21 1");legacy.replace(pos,8,"1 3 21");}
        const auto path=temp.dir/("v"+std::to_string(version)+".save");{std::ofstream out(path);out<<legacy;}
        check(SaveSystem::load(path,loaded,db,error)==LoadStatus::Loaded,"legacy version migrates");
        check(loaded.player.rodLevel()==5&&loaded.player.boxLevel()==5&&loaded.player.money()==data.player.money(),"legacy equipment and money preserved");
        for(std::size_t i=8;i<kUpgradeCount;++i)check(loaded.player.upgradeRank(kUpgrades[i].id)==0,"new stats begin at zero");
        check(SaveSystem::save(path,loaded,db,error),"legacy writes current version");
        check(std::filesystem::exists(path.string()+".bak"),"legacy backup preserved on migration");
    }
    check(SaveSystem::save(file,data,db,error),"backup created");{std::ofstream out(file);out<<"broken";}
    check(SaveSystem::load(file,loaded,db,error)==LoadStatus::RecoveredBackup&&loaded.player.upgradeRank(UpgradeId::Accuracy6)==3,"v3 backup recovers stats");
}
int main(){try{testTree();testGameplay();testSales();testSave();std::cout<<"Stats tests passed\n";return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
