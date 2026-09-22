#include "ui/UI.h"
#include "platform/WindowDrag.h"
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <vector>

void check(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }

int main() {
    struct TestWindow {
        TestWindow() {
            SetTraceLogLevel(LOG_WARNING);
            SetConfigFlags(FLAG_WINDOW_HIDDEN | FLAG_WINDOW_UNDECORATED | FLAG_MSAA_4X_HINT);
            InitWindow(UI::kWidth, UI::kCompactHeight, "Taskbar Fishing UI tests");
        }
        ~TestWindow() { CloseWindow(); }
    } window;
    try {
        check(IsWindowReady(), "real graphics context available");
        std::filesystem::create_directories("captures");
        FishingSystem fishing(42);
        Player player;
        UI ui;
        std::optional<FishInstance> last;
        bool autoSell = false;
        auto draw = [&](UIInput input = UIInput{}) {
            if (GetScreenHeight() != ui.windowHeight() || GetScreenWidth() != ui.windowWidth()) SetWindowSize(ui.windowWidth(), ui.windowHeight());
            BeginDrawing();
            auto action = ui.draw(fishing, player, last, autoSell, false, input);
            EndDrawing();
            return action;
        };
        auto click = [&](float x, float y, bool alt = false) {
            check(draw({{x, y}, true, false, false, alt}).type == UIActionType::None, "buttons act on release");
            return draw({{x, y}, false, true, false, alt});
        };
        auto rightClick = [&](float x, float y) { return draw({{x, y}, false, false, true, false}); };
        auto slotCenter = [](int slot) {
            const auto rect = UI::slotRect(slot);
            return Vector2{rect.x + rect.width / 2, rect.y + rect.height / 2};
        };
        auto capture = [&](const char* name, UIInput input = UIInput{}) {
            // Hidden double-buffered windows can expose the previous back buffer after a swap.
            draw(input); draw(input);
            struct ScreenImage {
                Image image = LoadImageFromScreen();
                ~ScreenImage() { UnloadImage(image); }
            } screen;
            check(screen.image.width == ui.windowWidth() && screen.image.height == ui.windowHeight(), "capture dimensions");
            check(ExportImage(screen.image, name), "export screenshot to requested path");
        };
        fishing.start(); fishing.update(0.65f);
        check(ui.height() == UI::kCompactHeight, "starts compact");
        capture("captures/compact.png");
        ui.beginOnboarding();
        draw();
        check(ui.onboardingOpen() && ui.height()==UI::kSettingsHeight && ui.onboardingStep()==0, "first launch opens the welcome screen");
        check(ui.canDrag({30,15}) && !ui.canDrag({256,240}), "welcome title is draggable and the name field is not");
        check(click(256,358).type==UIActionType::None && ui.onboardingStep()==0, "continue stays disabled until a name is entered");
        check(ui.setOnboardingName("Denki") && ui.onboardingName()=="Denki", "onboarding name can be set");
        capture("captures/welcome-name.png");
        check(click(256,358).type==UIActionType::None && ui.onboardingStep()==1, "continue advances to setup");
        capture("captures/welcome-settings.png");
        check(click(100,98).type==UIActionType::ToggleTopmost, "welcome pin control");
        check(click(100,218).type==UIActionType::ToggleAutoSell, "welcome auto sell control");
        check(click(132,358).type==UIActionType::None && ui.onboardingStep()==0, "back returns to the name step");
        check(ui.setOnboardingName("Denki") && click(256,358).type==UIActionType::None && ui.onboardingStep()==1, "name step can be completed again");
        check(click(378,358).type==UIActionType::FinishOnboarding, "start fishing finishes onboarding");
        ui.endOnboarding();
        check(!ui.onboardingOpen() && ui.height()==UI::kCompactHeight, "welcome closes to the compact lake");
        draw({{464,16},true});
        draw({{440,16},false,true});
        check(!ui.dashboardOpen(), "drag off expand cancels click");
        click(464,16);
        check(ui.dashboardOpen() && ui.height()==UI::kDashboardHeight, "expand opens portrait dashboard");
        capture("captures/ui-v3-expanded.png");
        check(ui.canDrag({30,22}) && ui.canDrag({250,80}) && ui.canDrag({200,420}), "dashboard title, portrait and lake are draggable");
        check(!ui.canDrag(slotCenter(0)) && !ui.canDrag({100,330}) && !ui.canDrag({492,376}), "inventory and buttons never start window drag");
        WindowDrag drag;
        auto moved=drag.update({120,130},{100,100},true,true,true);
        check(moved && moved->x==100 && moved->y==100, "drag starts without jumping");
        moved=drag.update({-30,300},{100,100},false,true,false);
        check(moved && moved->x==-50 && moved->y==270, "drag continues outside original region using screen coordinates");
        check(!drag.update({-30,300},{-50,270},false,false,false), "release ends dragging");
        check(!drag.update({120,130},{100,100},true,true,false), "buttons cannot initiate dragging");
        const auto originalPosition=GetWindowPosition();
        drag.update({originalPosition.x+20,originalPosition.y+30},originalPosition,true,true,true);
        const auto nativeMove=drag.update({originalPosition.x+50,originalPosition.y+50},originalPosition,false,true,false);
        SetWindowPosition(static_cast<int>(nativeMove->x),static_cast<int>(nativeMove->y));
        check(GetWindowPosition().x==originalPosition.x+30 && GetWindowPosition().y==originalPosition.y+20, "drag movement is applied to a real native window");
        SetWindowPosition(static_cast<int>(originalPosition.x),static_cast<int>(originalPosition.y));
        drag.cancel();
        click(100,330);
        check(ui.width()==808 && ui.dashboardX()==296, "craft opens on the left");
        capture("captures/ui-v3-crafting.png");
        check(click(144,314).type==UIActionType::None, "unfunded craft disabled");
        click(696,330);
        check(ui.width()==1104, "both sidebars can open");
        capture("captures/ui-v3-both.png");
        click(264,24);
        check(ui.width()==808 && ui.dashboardX()==0, "closing craft preserves stats");
        capture("captures/ui-v3-stats.png");
        check(click(630,329).type==UIActionType::None, "unfunded box upgrade disabled");
        click(640,503);
        check(ui.statsOpen(), "sidebar opens full progression tree");
        capture("captures/stats-unfunded.png");
        click(850,30);
        check(ui.dashboardOpen() && ui.width()==808, "tree returns to dashboard and sidebar");
        click(784,24);
        check(ui.width()==512, "stats closes independently");

        const FishInstance bass{2,3.72f,fishing.database().price(2,3.72f)};
        const FishInstance golden{4,2.5f,fishing.database().price(4,2.5f)};
        const FishInstance special{10,2,fishing.database().price(10,2)};
        for(const auto& fish:{bass,golden,special}) {player.recordCatch(fish);player.keep(fish);}
        last=bass;
        click(slotCenter(0).x,slotCenter(0).y);
        check(ui.selectedSlot()==0, "inventory selects first slot");
        capture("captures/fish-box.png");
        check(click(408,285).type==UIActionType::SellStored, "selected catch can be sold");
        auto action=click(464,285);
        check(action.type==UIActionType::ToggleLock, "lock button targets selection");
        player.toggleLock(action.index);
        check(rightClick(slotCenter(0).x,slotCenter(0).y).type==UIActionType::None, "protected catch ignores quick sell");
        check(click(408,285).type==UIActionType::None, "protected catch disables sell");
        action=click(slotCenter(0).x,slotCenter(0).y,true);
        check(action.type==UIActionType::ToggleLock, "alt click toggles protection");
        player.toggleLock(action.index);
        check(rightClick(slotCenter(0).x,slotCenter(0).y).type==UIActionType::SellStored, "right click sells unprotected catch");
        click(slotCenter(2).x,slotCenter(2).y);
        check(ui.selectedSlot()==2, "inventory selects special catch");
        capture("captures/ui-v3-special.png");
        action=click(464,285);player.toggleLock(action.index);
        check(click(420,200).type==UIActionType::SellAll, "sell all enabled");
        player.sellAll();
        check(player.fishBox().size()==1 && player.fishBox()[0].speciesId==10, "sell all keeps locked special");
        draw();
        check(!ui.selectedSlot(), "selection clears after sale");
        check(click(420,200).type==UIActionType::None, "sell all disabled when only protected items remain");
        click(260,200);
        check(ui.dashboardOpen() && ui.collectionTab(), "collection stays on the dashboard");
        for(int i=0;i<6;++i)draw({{250,250},false,false,false,false,false,-1});
        capture("captures/collection-specials.png");
        click(90,200);
        fishing.restoreCatch(special);last=special;
        capture("captures/caught-special.png");
        check(ui.dashboardOpen() && !ui.collectionTab(), "inventory tab restores the fish box");

        click(492,376);
        check(ui.settingsOpen() && !ui.boxOpen() && ui.height()==UI::kSettingsHeight, "settings replaces dashboard");
        check(ui.settingsTab()==0, "settings open on the general tab");
        ui.setSavePath("C:/Users/example/AppData/Local/TaskbarFishing/save.txt");
        capture("captures/settings.png");
        GamePreferences preferences;
        const auto id=[](PreferenceId value){return static_cast<std::size_t>(value);};
        // Clicks a settings row, applies the emitted preference to a local copy and mirrors it into the UI.
        auto pref=[&](float x,float y){
            const auto result=click(x,y);
            check(result.type==UIActionType::SetPreference && preferences.set(static_cast<PreferenceId>(result.index),result.value), "settings row emits a valid preference");
            ui.setPreferences(preferences);
            return result;
        };
        // Sliders react while the button is held, so press with the button down and release afterwards.
        auto sliderPress=[&](float x,float y){ const auto result=draw({{x,y},true,false,false,false,true}); draw({{x,y},false,true}); return result; };
        // General tab: rows are 30 px apart starting at y=84.
        check(pref(100,98).index==id(PreferenceId::DiscordPresence) && !preferences.discordPresence, "discord presence toggle");
        check(pref(100,128).index==id(PreferenceId::StartWithWindows) && preferences.startWithWindows, "start with windows toggle");
        check(pref(100,158).index==id(PreferenceId::ShowTooltips) && !preferences.showTooltips, "tooltip toggle");
        check(pref(100,188).index==id(PreferenceId::TwelveHourClock) && preferences.twelveHourClock, "clock format toggle");
        check(click(437,218).type==UIActionType::OpenSaveFolder, "open save folder control");
        preferences=GamePreferences{}; preferences.twelveHourClock=true; ui.setPreferences(preferences);
        // Window tab.
        click(159,56);
        check(ui.settingsTab()==1, "window tab selected");
        capture("captures/settings-window.png");
        check(click(100,98).type==UIActionType::ToggleTopmost, "pin control");
        check(pref(100,128).index==id(PreferenceId::LockWindow) && preferences.lockWindow, "lock setting creates preference action");
        check(!ui.canDrag({30,15}) && !ui.canDrag({200,300}), "lock setting disables dragging");
        preferences.lockWindow=false;ui.setPreferences(preferences);
        check(pref(100,158).index==id(PreferenceId::RememberPosition) && !preferences.rememberPosition, "remember position toggle");
        check(pref(261,188).index==id(PreferenceId::DockAlignment) && preferences.dockAlignment==0, "dock alignment left");
        check(pref(393,188).value==2, "dock alignment right");
        check(click(455,188).type==UIActionType::Dock, "dock control");
        check(click(396,22).type==UIActionType::ResetPosition, "recover position control");
        check(click(437,308).type==UIActionType::ResetPosition, "reset now control");
        check(click(430,22).type==UIActionType::Minimize, "minimize control");
        check(click(470,22).type==UIActionType::Close, "save and exit control");
        action=sliderPress(420,248);
        check(action.type==UIActionType::SetPreference && action.index==id(PreferenceId::Opacity) && action.value==90, "opacity slider snaps to 5% steps");
        preferences.opacity=90;ui.setPreferences(preferences);
        check(sliderPress(300,248).value==30, "opacity slider floor is 30%");
        preferences.opacity=30;ui.setPreferences(preferences);
        check(draw({{370,248},false,false,false,false,false,-1}).type==UIActionType::None, "wheel below the minimum does nothing");
        action=draw({{370,248},false,false,false,false,false,1});
        check(action.type==UIActionType::SetPreference && action.value==35, "wheel nudges the slider one step");
        preferences.opacity=100;ui.setPreferences(preferences);
        action=sliderPress(370,278);
        check(action.index==id(PreferenceId::IdleOpacity) && action.value==65, "idle opacity slider");
        action=sliderPress(370,218);
        check(action.index==id(PreferenceId::TaskbarGap) && action.value==24, "taskbar gap slider");
        // Display tab.
        click(257,56);
        capture("captures/settings-display.png");
        action=click(400,98);
        check(action.index==id(PreferenceId::Scale) && action.value==125, "scale choice");
        preferences.scale=125;ui.setPreferences(preferences);
        check(ui.windowWidth()==640 && ui.windowHeight()==685 && ui.scaled(100)==125, "scaled window dimensions");
        check(ui.toLogical({640,685}).x==512 && ui.toLogical({640,685}).y==548, "window coordinates map back to layout units");
        capture("captures/settings-scaled.png");
        action=click(344,98);
        check(action.index==id(PreferenceId::Scale) && action.value==100, "scale returns to 100%");
        preferences.scale=100;ui.setPreferences(preferences);
        check(click(344,128).value==30, "30 fps control");
        check(click(456,128).value==120, "120 fps control");
        check(pref(400,158).index==id(PreferenceId::BackgroundFps) && preferences.backgroundFps==30, "background frame rate choice");
        check(pref(100,188).index==id(PreferenceId::VSync) && preferences.vsync, "vsync toggle");
        check(pref(100,218).index==id(PreferenceId::ShowHud) && !preferences.showHud, "hud toggle");
        check(pref(100,248).index==id(PreferenceId::CatchNotices), "notices toggle");
        check(pref(100,278).index==id(PreferenceId::WaterEffects), "effects toggle");
        check(pref(100,308).index==id(PreferenceId::AnimateBackground) && !preferences.animateBackground, "background animation setting");
        const int frozenFrame=ui.backgroundFrame();ui.update(.5f);draw();
        check(ui.backgroundFrame()==frozenFrame, "animation toggle freezes actual background frames");
        preferences=GamePreferences{};ui.setPreferences(preferences);
        // Fishing tab.
        click(354,56);
        capture("captures/settings-fishing.png");
        check(click(100,98).type==UIActionType::ToggleAutoSell, "auto sell control");
        check(pref(100,128).index==id(PreferenceId::ProtectSpecial), "protect special control");
        check(pref(100,158).index==id(PreferenceId::ProtectRare) && preferences.protectRare, "protect rare control");
        check(pref(443,188).index==id(PreferenceId::BoxFullAction) && preferences.boxFullAction==2, "box full action choice");
        check(pref(100,218).index==id(PreferenceId::PauseUnfocused), "pause unfocused control");
        preferences=GamePreferences{};ui.setPreferences(preferences);
        // Hotkeys tab and footer.
        click(451,56);
        capture("captures/settings-hotkeys.png");
        check(pref(100,98).index==id(PreferenceId::Hotkeys) && !preferences.hotkeys, "keyboard shortcut toggle");
        preferences=GamePreferences{};ui.setPreferences(preferences);
        check(click(378,358).type==UIActionType::SaveNow, "save now control");
        check(click(132,358).type==UIActionType::ResetPreferences, "reset preferences does not reset progress");
        click(492,388);
        check(ui.height()==UI::kCompactHeight, "settings closes to compact");
        click(464,16);
        for(int i=0;i<300;++i)player.sell(golden);
        click(100,330);
        action=click(144,314);
        check(action.type==UIActionType::UpgradeStat && player.tryUpgrade(static_cast<UpgradeId>(action.index)), "craft buys selected equipment");
        check(click(144,314).type==UIActionType::None, "owned craft cannot be purchased twice");
        capture("captures/ui-v3-craft-owned.png");
        click(264,24);
        click(400,330);
        check(click(640,328).type==UIActionType::UpgradeBox, "stats sidebar upgrades capacity");
        check(click(640,460).type==UIActionType::UpgradeRod, "stats sidebar upgrades rod");
        click(640,503);
        auto selectNode=[&](UpgradeId id){const auto rect=ui.statsNodeRect(id);click(rect.x+rect.width/2,rect.y+rect.height/2);check(ui.selectedUpgrade()==id,"node selection");};
        selectNode(UpgradeId::Accuracy1);
        action=click(790,513);
        check(action.type==UIActionType::UpgradeStat && player.tryUpgrade(static_cast<UpgradeId>(action.index)), "progression purchase");
        capture("captures/stats-progress.png");
        const auto selected=ui.selectedUpgrade();
        draw({{350,300},true,false,false,false,true,0});draw({{280,250},false,false,false,false,true,0});draw({{280,250},false,true});
        check(ui.selectedUpgrade()==selected, "dragging tree does not select nodes");
        draw({{330,310},false,false,false,false,false,2});
        check(ui.statsZoom()>1, "tree zoom works");
        ui.backFromStats();
        while(player.upgradeBox()){}
        player.toggleLock(0);player.sellAll();
        while(player.keep(bass)){}
        click(784,24);
        click(slotCenter(24).x,slotCenter(24).y);
        action=click(408,285);
        check(action.type==UIActionType::SellStored && action.index==24, "last inventory slot is selectable");
        capture("captures/upgraded-box.png");
        check(ui.inventoryScroll()==0, "inventory starts at the first row");
        for(int i=0;i<4;++i)draw({{40,240},false,false,false,false,false,-1});
        check(ui.inventoryScroll()==4, "mouse wheel reveals extra inventory rows");
        const auto buried=UI::slotRect(40, ui.inventoryScroll());
        click(buried.x+buried.width/2, buried.y+buried.height/2);
        check(ui.selectedSlot()==40, "scrolled inventory slot is selectable");
        ui.closePanels();
        for(int hour:{0,6,12,18,23,0}) {
            ui.setLocalTime({hour,0,420});fishing.setLocalHour(hour);
            draw();ui.update(1);draw();
            check(ui.backgroundHour()==hour && ui.backgroundAvailable(),"hourly background including midnight");
            const std::string file=TextFormat("captures/game-%02dh.png",hour);
            capture(file.c_str());
        }
        ui.backFromStats(); ui.closePanels();
        player = Player(); fishing = FishingSystem(42); last.reset();
        ui.setLocalTime({12,0,0});
        draw(); ui.update(5); draw();
        const auto& catalog = AssetCatalog::get();
        check(catalog.sceneSize().x == UI::kWidth && catalog.sceneSize().y == UI::kSceneHeight, "window matches authored scene dimensions");
        const auto& cast = catalog.sprite("casting");
        check(catalog.fishId("Carp") == "common_carp" && catalog.fishId("Bass") == "largemouth_bass", "existing save species map to authored fish sprites");
        check(catalog.fishId("Old Boot").empty(), "non-fish catch keeps its own artwork");
        const auto& fishAnimation = catalog.sprite("fish/" + catalog.fishId("Bass"));
        check(fishAnimation.frameAt(120) == 1 && fishAnimation.frameAt(960) == 0, "fish swim animation uses supplied timing");
        check(cast.frameAt(119) == 0 && cast.frameAt(120) == 1 && cast.frameAt(320) == 3, "variable frame durations use millisecond boundaries");
        check(catalog.sprite("idle").frameAt(1400) == 0, "loop wraps exactly at animation duration");
        check(catalog.sprite("fx_water_impact").frameAt(740) == -1, "completed one-shot effects disappear");
        check(catalog.sprite("caught").frameAt(3000) == 3, "caught pose holds last frame");
        check(catalog.lighting(0).apply({100,150,200,73}).a == 73, "lighting preserves straight alpha");
        auto pixels = [&](Rectangle rect) {
            draw(); draw();
            Image screen = LoadImageFromScreen();
            std::vector<unsigned int> result;
            for (int y = static_cast<int>(rect.y); y < rect.y + rect.height; ++y)
                for (int x = static_cast<int>(rect.x); x < rect.x + rect.width; ++x) {
                    const auto color = GetImageColor(screen,x,y);
                    result.push_back(ColorToInt(color));
                }
            UnloadImage(screen);
            return result;
        };
        fishing.start(); fishing.update(.34f); ui.update(.34f); draw();
        check(ui.playerAnimation() == "casting" && ui.playerFrame() == 3, "cast sprite reaches release pose");
        capture("captures/sprite-casting.png");
        fishing.update(fishing.remaining()); draw();
        check(ui.playerAnimation() == "idle", "waiting uses idle sprite");
        const auto stillWater = pixels({350,35,154,70});
        const int oldFrame = ui.backgroundFrame();
        ui.update(.125f); draw();
        check(ui.backgroundFrame() == (oldFrame + 1) % 64, "background advances at 8 fps independently of fishing");
        check(stillWater != pixels({350,35,154,70}), "animated background changes rendered pixels");
        const auto stillPlayer = pixels({158,64,48,48});
        fishing.update(.7f); ui.update(.7f); draw();
        check(ui.playerFrame() == 1 && stillPlayer != pixels({158,64,48,48}), "idle animation changes rendered player pixels");
        const int playerFrame = ui.playerFrame(), backgroundFrame = ui.backgroundFrame();
        ui.setLocalTime({0,0,0}); draw();
        check(ui.playerFrame() == playerFrame && ui.backgroundFrame() == backgroundFrame, "hour change preserves both animation clocks");
        capture("captures/sprite-night.png");
        ui.setLocalTime({12,0,0}); draw(); ui.update(1); draw();
        fishing.update(fishing.remaining()); draw();
        check(ui.playerAnimation() == "bite", "bite sprite is selected");
        capture("captures/sprite-bite.png");
        fishing.update(fishing.remaining()); fishing.update(.3f); ui.update(.3f); draw();
        check(ui.playerAnimation() == "reeling" && ui.playerFrame() == 2, "reel sprite uses authored timing");
        capture("captures/sprite-reeling.png");
        bool landed = false, lost = false;
        for (int cycle = 0; cycle < 100 && (!landed || !lost); ++cycle) {
            fishing.update(fishing.remaining()); draw();
            if (fishing.state() == FishingState::Caught) {
                landed = true; check(ui.playerAnimation() == "caught", "landed sprite selected");
                fishing.update(.18f); ui.update(.18f); draw();
                capture("captures/sprite-lift.png");
                fishing.resolveCatch();
                draw();
                check(ui.playerAnimation() == "casting" && ui.playerFrame() == 0, "success restarts casting at first frame");
                fishing.update(.34f); ui.update(.34f); draw();
                check(ui.playerAnimation() == "casting" && ui.playerFrame() == 3, "success plays casting through release pose");
                capture("captures/sprite-recast-success.png");
            } else if (fishing.state() == FishingState::Escaped) {
                lost = true; check(ui.playerAnimation() == "escaped", "escaped sprite selected");
                capture("captures/sprite-escaped.png");
                fishing.update(.7f); draw();
                check(ui.playerAnimation() == "idle", "escaped animation returns to idle on completion");
                fishing.update(fishing.remaining()); draw();
                check(ui.playerAnimation() == "casting" && ui.playerFrame() == 0, "failure restarts casting at first frame");
                fishing.update(.34f); ui.update(.34f); draw();
                check(ui.playerAnimation() == "casting" && ui.playerFrame() == 3, "failure plays casting through release pose");
                capture("captures/sprite-recast-failure.png");
            }
        }
        check(landed && lost, "both landing and escape animations exercised");
        while (player.keep(bass)) {}
        fishing.restoreCatch(bass); draw();
        check(ui.playerAnimation() == "box_full", "full box selects waiting-with-catch pose");
        capture("captures/sprite-box-full.png");
        std::cout << "PASS: UI controls, inventory, upgrades, sprite timing, rendered animation, hourly lighting; screenshots in captures/\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
