#include "UI.h"
#include <algorithm>
#include <cmath>

namespace {
constexpr Color kInk{231,239,233,255}, kMuted{145,170,176,255}, kMint{135,216,189,255}, kGold{239,200,123,255};
// Settings panel geometry (logical pixels above the lake): title, five tabs, eight 30 px rows, footer.
constexpr float kRowHeight = 30, kFirstRow = 84, kRowRight = 482;
constexpr const char* kTabs[UI::kSettingsTabs]{"General","Window","Display","Fishing","Hotkeys"};
float rowY(int index) { return kFirstRow + index * kRowHeight; }
}

bool UI::canDrag(Vector2 mouse) const {
    if (preferences_.lockWindow) return false;
    if (statsOpen_) return CheckCollisionPointRec(mouse,dragRect());
    const float x=static_cast<float>(dashboardX());
    const float sceneTop=static_cast<float>(height()-kCompactHeight);
    if (CheckCollisionPointRec(mouse,{x,sceneTop,kWidth,kCompactHeight}) &&
        !CheckCollisionPointRec(mouse,{x+384,sceneTop,128,32})) return true;
    if (dashboardOpen()) {
        if (CheckCollisionPointRec(mouse,{x,0,512,32}) ||
            CheckCollisionPointRec(mouse,{x+204,48,104,96})) return true;
        if (craftOpen_ && CheckCollisionPointRec(mouse,{0,0,244,64})) return true;
        if (summaryOpen_ && CheckCollisionPointRec(mouse,{x+520,0,244,64})) return true;
    } else if ((extrasOpen_ || settingsOpen_ || onboardingOpen_) && CheckCollisionPointRec(mouse,{0,0,370,40})) return true;
    return false;
}

std::optional<int> UI::slider(Rectangle track, int value, int min, int max, int step) {
    const Rectangle hit{track.x-8,track.y-9,track.width+16,track.height+18};
    const bool hover=CheckCollisionPointRec(input_.mouse,hit);
    if (hover && input_.pressed) pressedButton_=hit;
    const bool active=pressedButton_ && pressedButton_->x==hit.x && pressedButton_->y==hit.y &&
        pressedButton_->width==hit.width && pressedButton_->height==hit.height;
    const float range=static_cast<float>(std::max(1,max-min));
    const float t=std::clamp((value-min)/range,0.0f,1.0f);
    skin_.progress(track,t,1);
    const float knobX=std::floor(track.x+t*track.width);
    DrawRectangleRounded({knobX-4,track.y-4,8,track.height+8},0.4f,4,active||hover?kMint:kInk);
    int next=value;
    if (active && (input_.down || input_.pressed)) {
        const float ratio=std::clamp((input_.mouse.x-track.x)/track.width,0.0f,1.0f);
        next=min+static_cast<int>(std::lround(ratio*range/step))*step;
    } else if (hover && input_.wheel!=0) next=value+(input_.wheel>0?step:-step);
    next=std::clamp(next,min,max);
    if (next!=value) return next;
    return std::nullopt;
}

UIAction UI::settingsPanel(bool autoSellCommon,bool topmost) {
    UIAction action;
    text("Settings",16,12,22,kInk);
    if(button({382,10,28,26},"",true,false,13))action.type=UIActionType::ResetPosition;
    if(button({416,10,28,26},"",true,false,14))action.type=UIActionType::Minimize;
    if(button({450,10,44,26},"",true,false,15))action.type=UIActionType::Close;
    if(CheckCollisionPointRec(input_.mouse,{382,10,28,26}))tooltip_="Reset window position (Shift + F12)";
    if(CheckCollisionPointRec(input_.mouse,{416,10,28,26}))tooltip_="Minimize";
    if(CheckCollisionPointRec(input_.mouse,{450,10,44,26}))tooltip_="Save & exit";
    for(int i=0;i<kSettingsTabs;++i)if(button({16+i*97.0f,44,92,25},kTabs[i],true,settingsTab_==i,-1,true))settingsTab_=i;
    card({16,76,478,262});

    const auto set=[&](PreferenceId id,int value){action={UIActionType::SetPreference,static_cast<std::size_t>(id),value};};
    const auto rowRect=[](float y){return Rectangle{22,y,466,28};};
    const auto hoverRow=[&](float y){
        const auto row=rowRect(y);
        if(CheckCollisionPointRec(input_.mouse,row))DrawRectangleLinesEx(row,1,Fade(kMint,.55f));
        return row;
    };
    // Title plus a muted hint; the hint is clipped before `controlLeft`, where the row's control begins.
    const auto label=[&](const char* title,const char* hint,float y,float controlLeft=455){
        text(title,28,y+2,13,kInk);
        if(hint&&*hint)fitText(hint,{28,y+15,controlLeft-36,12},10,kMuted);
    };
    const auto toggleRow=[&](const char* title,const char* hint,float y,bool value,PreferenceId id){
        const auto row=hoverRow(y);
        label(title,hint,y);
        skin_.draw(UISkin::Toggle,value?1:0,{455,y+8,24,12});
        if(clicked(row))set(id,!value);
    };
    const auto actionRow=[&](const char* title,const char* hint,float y,bool value,UIActionType type){
        const auto row=hoverRow(y);
        label(title,hint,y);
        skin_.draw(UISkin::Toggle,value?1:0,{455,y+8,24,12});
        if(clicked(row))action.type=type;
    };
    // Segmented choice: buttons of one width packed against the row's right edge.
    const auto choiceRow=[&](const char* title,const char* hint,float y,const char* const* options,int count,int selected,float width,float right,auto pick){
        label(title,hint,y,right-count*(width+4)+4);
        for(int i=0;i<count;++i){
            const Rectangle rect{right-(count-i)*(width+4)+4,y+3,width,22};
            if(button(rect,options[i],true,selected==i))pick(i);
        }
    };
    const auto sliderRow=[&](const char* title,const char* hint,float y,int value,int min,int max,int step,const std::string& display,PreferenceId id){
        label(title,hint,y,292);
        if(const auto next=slider({300,y+11,140,6},value,min,max,step))set(id,*next);
        text(display.c_str(),450,y+5,13,kInk);
    };
    const auto infoRow=[&](const char* title,const char* hint,float y,const std::string& value,Color color){
        const float valueX=kRowRight-textWidth(value.c_str(),13);
        label(title,hint,y,valueX);
        text(value.c_str(),valueX,y+6,13,color);
    };

    if(settingsTab_==0) {
        toggleRow("Discord Rich Presence","Show your fishing status in Discord while the game runs.",rowY(0),preferences_.discordPresence,PreferenceId::DiscordPresence);
        toggleRow("Start with Windows","Launch Taskbar Fishing when you sign in.",rowY(1),preferences_.startWithWindows,PreferenceId::StartWithWindows);
        toggleRow("Show tooltips","Hints when hovering buttons, slots and fish.",rowY(2),preferences_.showTooltips,PreferenceId::ShowTooltips);
        toggleRow("12-hour clock","Show the lake clock as 6:00 PM instead of 18:00.",rowY(3),preferences_.twelveHourClock,PreferenceId::TwelveHourClock);
        label("Save file",savePath_.empty()?"Save file location":savePath_.c_str(),rowY(4),392);
        if(button({392,rowY(4)+3,90,22},"Open folder"))action.type=UIActionType::OpenSaveFolder;
        infoRow("Local time","Catch odds follow the Windows clock and time zone.",rowY(5),localTime_.label(),kMint);
        infoRow("About","Taskbar Fishing prototype - C++20, raylib 5.5.",rowY(6),"0.1",kMuted);
    } else if(settingsTab_==1) {
        actionRow("Always on top","Keep the lake above other windows.",rowY(0),topmost,UIActionType::ToggleTopmost);
        toggleRow("Lock window position","Disable dragging. Settings stay available.",rowY(1),preferences_.lockWindow,PreferenceId::LockWindow);
        toggleRow("Remember window position","Restore the last position on startup.",rowY(2),preferences_.rememberPosition,PreferenceId::RememberPosition);
        constexpr const char* positions[]{"Left","Center","Right"};
        choiceRow("Dock position","Dock snaps the lake above the taskbar.",rowY(3),positions,3,preferences_.dockAlignment,62,424,[&](int i){set(PreferenceId::DockAlignment,i);});
        if(button({428,rowY(3)+3,54,22},"Dock"))action.type=UIActionType::Dock;
        sliderRow("Gap above taskbar","Space kept between the lake and the taskbar when docked.",rowY(4),preferences_.taskbarGap,0,48,4,TextFormat("%d px",preferences_.taskbarGap),PreferenceId::TaskbarGap);
        sliderRow("Opacity","Window transparency while you are using it.",rowY(5),preferences_.opacity,30,100,5,TextFormat("%d%%",preferences_.opacity),PreferenceId::Opacity);
        sliderRow("Opacity when idle","Fades when unfocused and the cursor is away. 100% disables.",rowY(6),preferences_.idleOpacity,30,100,5,TextFormat("%d%%",preferences_.idleOpacity),PreferenceId::IdleOpacity);
        label("Recover window","Shift + F12 snaps the lake back onto the primary monitor.",rowY(7),392);
        if(button({392,rowY(7)+3,90,22},"Reset now"))action.type=UIActionType::ResetPosition;
    } else if(settingsTab_==2) {
        constexpr const char* scales[]{"75%","100%","125%","150%"};
        constexpr int scaleValues[]{75,100,125,150};
        const int scaleIndex=static_cast<int>(std::find(std::begin(scaleValues),std::end(scaleValues),preferences_.scale)-std::begin(scaleValues));
        choiceRow("Window scale","Shift + F11 restores 100% scale and full opacity.",rowY(0),scales,4,scaleIndex,52,kRowRight,[&](int i){set(PreferenceId::Scale,scaleValues[i]);});
        constexpr const char* rates[]{"30","60","120"};
        constexpr int rateValues[]{30,60,120};
        const int rateIndex=static_cast<int>(std::find(std::begin(rateValues),std::end(rateValues),preferences_.fps)-std::begin(rateValues));
        choiceRow("Frame rate","Frames per second while the lake is active.",rowY(1),rates,3,rateIndex,52,kRowRight,[&](int i){set(PreferenceId::Fps,rateValues[i]);});
        constexpr const char* background[]{"Same","30","15"};
        constexpr int backgroundValues[]{0,30,15};
        const int backgroundIndex=static_cast<int>(std::find(std::begin(backgroundValues),std::end(backgroundValues),preferences_.backgroundFps)-std::begin(backgroundValues));
        choiceRow("Background frame rate","Lower cap while unfocused and the cursor is away.",rowY(2),background,3,backgroundIndex,52,kRowRight,[&](int i){set(PreferenceId::BackgroundFps,backgroundValues[i]);});
        toggleRow("VSync","Sync frames to the monitor refresh rate.",rowY(3),preferences_.vsync,PreferenceId::VSync);
        toggleRow("Show clock and catch status","Time, hourly catch chance and fishing state over the lake.",rowY(4),preferences_.showHud,PreferenceId::ShowHud);
        toggleRow("Catch and discovery notices","Announce new species and personal bests.",rowY(5),preferences_.catchNotices,PreferenceId::CatchNotices);
        toggleRow("Water and casting effects","Ripples, splashes and the cast swoosh.",rowY(6),preferences_.waterEffects,PreferenceId::WaterEffects);
        toggleRow("Animate lake background","Freeze the scenery without stopping the clock or fishing.",rowY(7),preferences_.animateBackground,PreferenceId::AnimateBackground);
    } else if(settingsTab_==3) {
        actionRow("Auto sell common catches","Bluegill, Carp and Old Boot sell as soon as they are landed.",rowY(0),autoSellCommon,UIActionType::ToggleAutoSell);
        toggleRow("Protect special catches","Lock Special finds automatically when they enter the box.",rowY(1),preferences_.protectSpecial,PreferenceId::ProtectSpecial);
        toggleRow("Protect rare catches","Lock Rare, Epic and Legendary catches automatically.",rowY(2),preferences_.protectRare,PreferenceId::ProtectRare);
        constexpr const char* fullActions[]{"Wait","Sell catch","Sell cheapest"};
        choiceRow("When the Fish Box is full","Free a slot instead of waiting for you.",rowY(3),fullActions,3,preferences_.boxFullAction,78,kRowRight,[&](int i){set(PreferenceId::BoxFullAction,i);});
        toggleRow("Pause fishing when unfocused","Stop the bite timer while another window has focus.",rowY(4),preferences_.pauseUnfocused,PreferenceId::PauseUnfocused);
        infoRow("Special encounter rate","Fixed chance per bite; rod and stats do not change it.",rowY(5),"0.1%",kMint);
        infoRow("Local time","Catch odds follow the Windows clock and time zone.",rowY(6),localTime_.label(),kMuted);
    } else {
        toggleRow("Keyboard shortcuts","Esc and the Shift + F11 / F12 recovery keys always work.",rowY(0),preferences_.hotkeys,PreferenceId::Hotkeys);
        struct Hotkey { const char* keys; const char* effect; };
        constexpr Hotkey left[]{{"B / C","Fish Box / Collection"},{"U","Stats tree"},{"S / L","Sell / lock selection"},
                                {"A","Auto sell common"},{"R / F","Upgrade rod / box"},{"T / D","Always on top / Dock"}};
        constexpr Hotkey right[]{{"Esc","Close panels"},{"Alt + click","Lock a catch"},{"Right click","Quick sell"},
                                 {"Shift + F12","Reset position"},{"Shift + F11","Reset scale"},{"Alt + F4","Save & exit"}};
        const Color keyColor=preferences_.hotkeys?kMint:kMuted;
        for(int i=0;i<6;++i) {
            const float y=rowY(1+i)+6;
            text(left[i].keys,28,y,12,keyColor);
            text(left[i].effect,118,y,11,kMuted);
            text(right[i].keys,268,y,12,i<3?kMint:kGold);
            text(right[i].effect,358,y,11,kMuted);
        }
    }
    if(button({16,346,232,24},"Reset settings"))action.type=UIActionType::ResetPreferences;
    if(button({262,346,232,24},"Save now"))action.type=UIActionType::SaveNow;
    if(CheckCollisionPointRec(input_.mouse,{16,346,232,24}))tooltip_="Restore default settings. Fish, money and upgrades are kept.";
    return action;
}
