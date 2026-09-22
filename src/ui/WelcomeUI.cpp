#include "UI.h"
#include "AssetTexture.h"
#include <algorithm>
#include <cmath>

namespace {
constexpr Color kInk{231,239,233,255}, kMuted{145,170,176,255}, kMint{135,216,189,255}, kGold{239,200,123,255};
constexpr float kRowHeight = 30, kFirstRow = 84, kRowRight = 482;
float rowY(int index) { return kFirstRow + index * kRowHeight; }
}

void UI::beginOnboarding() {
    closePanels();
    statsOpen_ = false;
    onboardingOpen_ = true;
    onboardingStep_ = 0;
    onboardingName_.clear();
    nameFocused_ = true;
    pressedButton_.reset();
}

void UI::endOnboarding() {
    onboardingOpen_ = false;
    onboardingStep_ = 0;
    nameFocused_ = false;
    pressedButton_.reset();
}

void UI::onboardingBack() {
    if (!onboardingOpen_ || onboardingStep_ <= 0) return;
    onboardingStep_ = 0;
    nameFocused_ = true;
    pressedButton_.reset();
}

bool UI::setOnboardingName(std::string value) {
    while (!value.empty() && (value.front() == ' ' || value.front() == '\t')) value.erase(value.begin());
    while (!value.empty() && (value.back() == ' ' || value.back() == '\t')) value.pop_back();
    if (!validPlayerName(value, true)) return false;
    onboardingName_ = std::move(value);
    return true;
}

void UI::consumeNameInput() {
    if (!nameFocused_ || !IsWindowFocused()) return;
    if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) utf8PopLast(onboardingName_);
    const bool ctrl = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
    if (ctrl && IsKeyPressed(KEY_V)) {
        if (const char* clip = GetClipboardText()) {
            std::string next = onboardingName_;
            for (unsigned char byte : std::string(clip)) {
                if (byte < 32 || byte == 127) { next.clear(); break; }
                next += static_cast<char>(byte);
            }
            if (!next.empty() && validPlayerName(next, true)) onboardingName_ = std::move(next);
        }
    }
    for (;;) {
        const int codepoint = GetCharPressed();
        if (codepoint == 0) break;
        if (codepoint == '\n' || codepoint == '\r') continue;
        utf8AppendCodepoint(onboardingName_, codepoint);
    }
    if ((IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) && validPlayerName(onboardingName_, false)) {
        onboardingStep_ = 1;
        nameFocused_ = false;
    }
}

UIAction UI::welcomePanel(bool autoSellCommon, bool topmost) {
    UIAction action;
    text(onboardingStep_ == 0 ? "Welcome" : "Setup", 16, 12, 22, kInk);
    if (button({416,10,28,26},"",true,false,14)) action.type = UIActionType::Minimize;
    if (button({450,10,44,26},"",true,false,15)) action.type = UIActionType::Close;
    if (CheckCollisionPointRec(input_.mouse,{416,10,28,26})) tooltip_ = "Minimize";
    if (CheckCollisionPointRec(input_.mouse,{450,10,44,26})) tooltip_ = "Exit without saving";

    if (!portrait_.id) {
        portrait_ = loadAssetTexture("assets/player/shoulder_portrait.png");
        if (portrait_.id) SetTextureFilter(portrait_, TEXTURE_FILTER_POINT);
    }

    if (onboardingStep_ == 0) {
        consumeNameInput();
        card({184,44,144,144},{15,26,35,255});
        if (portrait_.id) DrawTexturePro(portrait_,{0,0,static_cast<float>(portrait_.width),static_cast<float>(portrait_.height)},{184,44,144,144},{0,0},0,WHITE);
        text("What should we call you?", 16, 200, 16, kInk);
        const Rectangle field{16, 226, 480, 32};
        const bool hover = CheckCollisionPointRec(input_.mouse, field);
        card(field, nameFocused_ || hover ? Color{31,51,61,255} : Color{23,39,49,255}, nameFocused_ ? kMint : Color{48,70,79,255});
        if (clicked(field)) nameFocused_ = true;
        if (onboardingName_.empty()) text("Your name", 26, 233, 14, kMuted);
        else fitText(onboardingName_, {26, 226, 430, 32}, 16, kInk);
        if (nameFocused_ && std::fmod(GetTime(), 1.0) < 0.55) {
            const float caret = 26 + (onboardingName_.empty() ? 0 : std::min(420.0f, textWidth(onboardingName_.c_str(), 16)));
            DrawRectangle(static_cast<int>(caret), 232, 1, 18, kMint);
        }
        text("Up to 16 characters. You can keep fishing after this.", 16, 266, 12, kMuted);
        const bool ready = validPlayerName(onboardingName_, false);
        if (button({16,346,480,24}, "Continue", ready) && ready) {
            onboardingStep_ = 1;
            nameFocused_ = false;
        }
        return action;
    }

    text(TextFormat("Hi, %s. Set up your lake.", onboardingName_.c_str()), 16, 46, 14, kMuted);
    card({16,76,478,262});

    const auto set=[&](PreferenceId id,int value){action={UIActionType::SetPreference,static_cast<std::size_t>(id),value};};
    const auto rowRect=[](float y){return Rectangle{22,y,466,28};};
    const auto hoverRow=[&](float y){
        const auto row=rowRect(y);
        if(CheckCollisionPointRec(input_.mouse,row))DrawRectangleLinesEx(row,1,Fade(kMint,.55f));
        return row;
    };
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
    const auto choiceRow=[&](const char* title,const char* hint,float y,const char* const* options,int count,int selected,float width,float right,auto pick){
        label(title,hint,y,right-count*(width+4)+4);
        for(int i=0;i<count;++i){
            const Rectangle rect{right-(count-i)*(width+4)+4,y+3,width,22};
            if(button(rect,options[i],true,selected==i))pick(i);
        }
    };

    actionRow("Always on top","Keep the lake above other windows.",rowY(0),topmost,UIActionType::ToggleTopmost);
    toggleRow("Start with Windows","Launch Taskbar Fishing when you sign in.",rowY(1),preferences_.startWithWindows,PreferenceId::StartWithWindows);
    toggleRow("Discord Rich Presence","Show your fishing status in Discord.",rowY(2),preferences_.discordPresence,PreferenceId::DiscordPresence);
    toggleRow("12-hour clock","Show the lake clock as 6:00 PM instead of 18:00.",rowY(3),preferences_.twelveHourClock,PreferenceId::TwelveHourClock);
    actionRow("Auto sell common catches","Bluegill, Carp and Old Boot sell as soon as they are landed.",rowY(4),autoSellCommon,UIActionType::ToggleAutoSell);
    constexpr const char* scales[]{"75%","100%","125%","150%"};
    constexpr int scaleValues[]{75,100,125,150};
    const int scaleIndex=static_cast<int>(std::find(std::begin(scaleValues),std::end(scaleValues),preferences_.scale)-std::begin(scaleValues));
    choiceRow("Window scale","Size of the lake on your screen.",rowY(5),scales,4,scaleIndex,52,kRowRight,[&](int i){set(PreferenceId::Scale,scaleValues[i]);});
    constexpr const char* rates[]{"30","60","120"};
    constexpr int rateValues[]{30,60,120};
    const int rateIndex=static_cast<int>(std::find(std::begin(rateValues),std::end(rateValues),preferences_.fps)-std::begin(rateValues));
    choiceRow("Frame rate","Frames per second while the lake is active.",rowY(6),rates,3,rateIndex,52,kRowRight,[&](int i){set(PreferenceId::Fps,rateValues[i]);});
    constexpr const char* positions[]{"Left","Center","Right"};
    choiceRow("Dock position","Where the lake sits above the taskbar.",rowY(7),positions,3,preferences_.dockAlignment,62,kRowRight,[&](int i){set(PreferenceId::DockAlignment,i);});

    if (button({16,346,232,24},"Back")) onboardingBack();
    const bool ready = validPlayerName(onboardingName_, false);
    if (button({262,346,232,24},"Start fishing", ready) && ready) action.type = UIActionType::FinishOnboarding;
    return action;
}
