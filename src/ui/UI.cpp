#include "ui/UI.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <vector>

namespace {
constexpr Color kBackground{15, 26, 35, 255};
constexpr Color kRaised{31, 51, 61, 255};
constexpr Color kBorder{48, 70, 79, 255};
constexpr Color kText{231, 239, 233, 255};
constexpr Color kMuted{145, 170, 176, 255};
constexpr Color kMint{135, 216, 189, 255};
constexpr Color kGold{239, 200, 123, 255};
constexpr Color kAlert{242, 154, 129, 255};
constexpr int kGridX = 16, kGridY = 218, kCell = 28, kStride = 34;

Color rarityColor(FishRarity rarity) {
    switch (rarity) {
    case FishRarity::Common: return {182, 202, 207, 255};
    case FishRarity::Uncommon: return kMint;
    case FishRarity::Rare: return {133, 184, 248, 255};
    case FishRarity::Epic: return {197, 158, 240, 255};
    case FishRarity::Legendary: return kGold;
    case FishRarity::Special: return {233,160,188,255};
    }
    return kText;
}
Color speciesColor(int id) {
    static constexpr std::array<Color, 6> colors{{
        {115, 181, 219, 255}, {211, 174, 117, 255}, {135, 192, 148, 255},
        {163, 160, 184, 255}, {247, 207, 104, 255}, {167, 123, 91, 255},
    }};
    return id >= 0 && static_cast<std::size_t>(id) < colors.size() ? colors[static_cast<std::size_t>(id)] : kMuted;
}
std::string moneyLabel(std::int64_t money) {
    if (money < 100000) return TextFormat("$%lld", static_cast<long long>(money));
    if (money < 1000000) return TextFormat("$%.1fk", money / 1000.0);
    if (money < 1000000000) return TextFormat("$%.1fm", money / 1000000.0);
    return TextFormat("$%.1fb", money / 1000000000.0);
}
}

UI::~UI() { releaseGraphics(); }
void UI::card(Rectangle rect, Color fill, Color border, float) const {
    if (!skin_.draw(UISkin::Panel, fill.r == kBackground.r ? 1 : 0, rect)) {
        DrawRectangleRec(rect, fill);
        DrawRectangleLinesEx(rect, 1, border);
    }
}
void UI::track(Rectangle rect, float progress, Color accent) const {
    skin_.progress(rect, progress, accent.r == kAlert.r ? 3 : accent.r == kGold.r ? 2 : 1);
}
void UI::initFont() {
    if (fontReady_) return;
    font_ = GetFontDefault();
    // Use Windows' UI font without bundling or redistributing a system font.
    if (const char* windows = std::getenv("WINDIR")) {
        const std::string path = std::string(windows) + "/Fonts/segoeui.ttf";
        if (FileExists(path.c_str())) {
            Font loaded{};
            std::vector<int> codepoints;
            codepoints.reserve(400);
            for (int i = 32; i < 127; ++i) codepoints.push_back(i);
            for (int i = 160; i < 256; ++i) codepoints.push_back(i);
            for (int i : {0x0102, 0x0103, 0x0110, 0x0111, 0x01A0, 0x01A1, 0x01AF, 0x01B0}) codepoints.push_back(i);
            for (int i = 0x1EA0; i <= 0x1EF9; ++i) codepoints.push_back(i);
            loaded = LoadFontEx(path.c_str(), 32, codepoints.data(), static_cast<int>(codepoints.size()));
            if (loaded.texture.id != 0 && loaded.texture.id != font_.texture.id) {
                font_ = loaded;
                ownsFont_ = true;
                SetTextureFilter(font_.texture, TEXTURE_FILTER_BILINEAR);
            }
        }
    }
    fontReady_ = true;
}
void UI::releaseGraphics() {
    if (ownsFont_ && IsWindowReady()) UnloadFont(font_);
    background_.release();
    skin_.release();
    sprites_.release();
    if (portrait_.id && IsWindowReady()) UnloadTexture(portrait_);
    portrait_ = {};
    font_ = {};
    ownsFont_ = fontReady_ = false;
}
void UI::text(const char* label, float x, float y, float size, Color color) const {
    // Segoe's em includes generous ascender space; normalize its visible size for this compact UI.
    DrawTextEx(font_, label, {x, y - (ownsFont_ ? 2.0f : 0.0f)}, size * (ownsFont_ ? 1.2f : 1.0f), ownsFont_ ? 0.0f : 1.0f, color);
}
float UI::textWidth(const char* label, float size) const {
    return MeasureTextEx(font_, label, size * (ownsFont_ ? 1.2f : 1.0f), ownsFont_ ? 0.0f : 1.0f).x;
}
void UI::fitText(const std::string& label, Rectangle rect, float size, Color color) const {
    std::string display = label;
    if (textWidth(display.c_str(), size) > rect.width) {
        while (!display.empty() && textWidth((display + "...").c_str(), size) > rect.width) display.pop_back();
        display += "...";
    }
    text(display.c_str(), rect.x, rect.y + (rect.height - size) / 2, size, color);
}
Rectangle UI::slotRect(int slot, int scrollRows) {
    return {static_cast<float>(kGridX + (slot % kGridCols) * kStride),
            static_cast<float>(kGridY + (slot / kGridCols - scrollRows) * kStride),
            static_cast<float>(kCell), static_cast<float>(kCell)};
}
void UI::update(float dt) { noticeSeconds_ = std::max(0.0f, noticeSeconds_ - dt); frameDt_ = dt; }
bool UI::clicked(Rectangle rect) {
    const bool hover = CheckCollisionPointRec(input_.mouse, rect);
    if (hover && input_.pressed) pressedButton_ = rect;
    const bool active = pressedButton_ && pressedButton_->x == rect.x && pressedButton_->y == rect.y &&
        pressedButton_->width == rect.width && pressedButton_->height == rect.height;
    return hover && active && input_.released;
}
bool UI::button(Rectangle rect, const std::string& label, bool enabled, bool selected, int icon, bool tab) {
    const bool hover = CheckCollisionPointRec(input_.mouse, rect);
    const bool hit = enabled && clicked(rect);
    const bool held = enabled && hover && !input_.released && pressedButton_ && pressedButton_->x == rect.x && pressedButton_->y == rect.y;
    const Color fill = !enabled ? Color{27, 40, 48, 255} : held ? Color{56, 105, 96, 255} :
        selected ? Color{43, 83, 76, 255} : hover ? Color{44, 69, 77, 255} : kRaised;
    const int state = tab ? (!enabled ? 3 : selected ? 2 : hover ? 1 : 0)
                          : (!enabled ? 4 : held ? 2 : selected ? 3 : hover ? 1 : 0);
    if (!skin_.draw(tab ? UISkin::Tab : UISkin::Button, state, rect)) card(rect, fill);
    const Color ink = !enabled ? kMuted : selected ? kGold : kText;
    const float offset = held ? 1.0f : 0.0f;
    if (icon >= 0) {
        const float x = label.empty() ? rect.x + (rect.width - 16) / 2 : rect.x + 7;
        skin_.draw(UISkin::Icons, icon, {std::floor(x), std::floor(rect.y + (rect.height - 16) / 2 + offset), 16, 16}, enabled ? WHITE : kMuted);
    }
    if (!label.empty()) {
        const float left = icon >= 0 ? 28.0f : 6.0f;
        const float available = rect.width - left - 6;
        float size = 14;
        while (size > 10 && textWidth(label.c_str(), size) > available) size -= 1;
        const float width = std::min(available, textWidth(label.c_str(), size));
        fitText(label, {rect.x + left + (available - width) / 2, rect.y - 1 + offset, width, rect.height}, size, ink);
    }
    return hit;
}

void UI::scene(const FishingSystem& fishing, const Player& player, bool autoSellCommon) {
    if (!background_.draw({0, 0, kWidth, kSceneHeight})) DrawRectangle(0, 0, kWidth, kSceneHeight, kBackground);
    const auto state = fishing.state();
    sprites_.draw(fishing, [&](Rectangle rect) {
        if (fishing.catchResult()) fishIcon(*fishing.catchResult(), fishing.database().at(fishing.catchResult()->speciesId), rect);
    });
    if (!preferences_.showHud) return;
    const auto conditions = conditionsForHour(localTime_.hour);
    const Color chanceColor = conditions.successPercent >= 70 ? kMint : conditions.successPercent < 40 ? kAlert : kGold;
    card({8, 4, 225, 24});
    const int hour12 = localTime_.hour % 12 == 0 ? 12 : localTime_.hour % 12;
    const std::string clock = preferences_.twelveHourClock
        ? TextFormat("%d:%02d %s", hour12, localTime_.minute, localTime_.hour < 12 ? "AM" : "PM")
        : TextFormat("%02d:%02d", localTime_.hour, localTime_.minute);
    text(clock.c_str(), 16, 9, 12, kText);
    const float chanceX = 16 + std::ceil(textWidth(clock.c_str(), 12)) + 9;
    fitText(TextFormat("Catch %.1f%% / %s", player.stats().successChance(conditions.successPercent/100.f)*100, conditions.level), {chanceX, 4, 226 - chanceX, 22}, 12, chanceColor);
    std::string status = FishingSystem::stateName(state);
    if (state == FishingState::Waiting) status = TextFormat("Next bite %.1fs", fishing.remaining());
    if (fishing.catchSettled() && player.boxFull() && !fishing.shouldAutoSell(autoSellCommon))
        status = "Fish Box full";
    else if (preferences_.catchNotices && noticeSeconds_ > 0 && (milestone_.newSpecies || milestone_.newRecord))
        status = milestone_.newSpecies ? "New discovery!" : "Personal best!";
    card({350, 115, 154, 23});
    fitText(status, {359, 118, 136, 17}, 12, state == FishingState::Escaped ? kAlert : kText);
}

void UI::fishIcon(const FishInstance& fish, const FishSpecies& species, Rectangle cell) {
    if (sprites_.drawIcon(species.name, cell)) return;
    const float cx = cell.x + cell.width / 2, cy = cell.y + cell.height / 2;
    const Color color = speciesColor(fish.speciesId);
    if (species.name == "Old Boot") {
        DrawRectangleRounded({cx - 5, cy - 8, 7, 13}, 0.2f, 4, color);
        DrawRectangleRounded({cx - 5, cy + 1, 13, 6}, 0.4f, 4, color);
        DrawRectangle(static_cast<int>(cx) - 5, static_cast<int>(cy) + 6, 13, 2, {81, 66, 53, 255});
        DrawLine(static_cast<int>(cx) - 4, static_cast<int>(cy) - 6, static_cast<int>(cx) + 1, static_cast<int>(cy) - 6, kGold);
        return;
    }
    const float range = std::max(0.01f, species.maxWeight - species.minWeight);
    const float size = std::clamp((fish.weight - species.minWeight) / range, 0.0f, 1.0f);
    const float body = 5.0f + 3.0f * size, bx = cx + 2;
    DrawTriangle({bx - body + 1, cy}, {bx - body - 4, cy - 4}, {bx - body - 4, cy + 4}, color);
    DrawTriangle({bx - 3, cy - 2}, {bx + 3, cy - 2}, {bx, cy - 6}, Fade(color, 0.8f));
    DrawEllipse(static_cast<int>(bx), static_cast<int>(cy), body, body * 0.55f, color);
    DrawLine(static_cast<int>(bx) - 2, static_cast<int>(cy) + 2, static_cast<int>(bx) + 3, static_cast<int>(cy) + 2, Fade(kText, 0.45f));
    DrawCircle(static_cast<int>(bx + body * 0.5f), static_cast<int>(cy) - 1, 1.2f, kBackground);
}
void UI::itemTooltip(const FishInstance& fish, const FishSpecies& species, const Player& player) const {
    const float width = 190, tipHeight = fish.locked ? 85.0f : 66.0f;
    const float x = std::clamp(input_.mouse.x + 14, 4.0f, static_cast<float>(this->width()) - width - 4);
    const float y = std::clamp(input_.mouse.y + 10, 4.0f, height() - tipHeight - 4);
    const Color accent = rarityColor(species.rarity);
    DrawRectangleRounded({x + 3, y + 4, width, tipHeight}, 0.15f, 8, {0, 0, 0, 70});
    card({x, y, width, tipHeight}, kBackground, Fade(accent, 0.6f));
    text(species.name.c_str(), x + 10, y + 7, 16, accent);
    text(TextFormat("%s  /  %.2f kg", rarityName(species.rarity), fish.weight), x + 10, y + 29, 13, kMuted);
    text(TextFormat("Sell value   $%lld", static_cast<long long>(player.salePrice(fish))), x + 10, y + 47, 13, kGold);
    if (fish.locked) text("Locked  /  Alt + click to unlock", x + 10, y + 66, 12, kAlert);
}

UIAction UI::fishBoxPanel(const FishingSystem& fishing, const Player& player) {
    UIAction action;
    const auto& box = player.fishBox();
    if (selected_ && *selected_ >= box.size()) selected_.reset();
    text("Your fish box", 28, 83, 16, kText);
    text(TextFormat("%d / %d", static_cast<int>(box.size()), player.capacity()), 148, 86, 12, player.boxFull() ? kAlert : kMuted);
    text("INSPECT", 316, 86, 11, kMuted);
    int lockedCount = 0;
    const int first = inventoryScroll_ * kGridCols;
    const int visible = kGridCols * kVisibleRows;
    const int rows = (player.capacity() + kGridCols - 1) / kGridCols;
    const int maxScroll = std::max(0, rows - kVisibleRows);
    if (CheckCollisionPointRec(input_.mouse, {16, 76, 478, 142}) && input_.wheel != 0)
        inventoryScroll_ -= input_.wheel > 0 ? 1 : -1;
    inventoryScroll_ = std::clamp(inventoryScroll_, 0, maxScroll);
    for (int i = 0; i < visible; ++i) {
        const int slot = first + i;
        if (slot >= kGridSlots) break;
        const auto rect = slotRect(slot, inventoryScroll_);
        const bool available = slot < player.capacity();
        const bool filled = static_cast<std::size_t>(slot) < box.size();
        const bool hover = CheckCollisionPointRec(input_.mouse, rect);
        if (hover) tooltip_ = available ? "Click: select   /   Right click: sell   /   Alt + click: lock" : "Upgrade your Fish Box to unlock more slots.";
        const auto rarity = filled ? fishing.database().at(box[static_cast<std::size_t>(slot)].speciesId).rarity : FishRarity::Common;
        const int slotState = !available ? 1 : !filled ? 0 : rarity == FishRarity::Common ? 2 : rarity == FishRarity::Uncommon ? 3 : 4;
        skin_.draw(UISkin::Slot, slotState, rect);
        if (!available || !filled) continue;
        if (rarity == FishRarity::Epic || rarity == FishRarity::Legendary || rarity == FishRarity::Special)
            DrawRectangleLinesEx(rect, 1, rarityColor(rarity));
        const auto& fish = box[static_cast<std::size_t>(slot)];
        const auto& species = fishing.database().at(fish.speciesId);
        if (fish.locked) ++lockedCount;
        fishIcon(fish, species, rect);
        if (hover) skin_.draw(UISkin::Overlay, 0, rect);
        if (selected_ && *selected_ == static_cast<std::size_t>(slot)) skin_.draw(UISkin::Overlay, 1, rect);
        if (fish.locked) skin_.draw(UISkin::Overlay, 2, rect);
        if (hover) hovered_ = static_cast<std::size_t>(slot);
        if (hover && input_.rightPressed && !fish.locked) action = {UIActionType::SellStored, static_cast<std::size_t>(slot)};
        if (clicked(rect)) {
            if (input_.altDown) action = {UIActionType::ToggleLock, static_cast<std::size_t>(slot)};
            else selected_ = static_cast<std::size_t>(slot);
        }
    }
    DrawLine(301, 109, 301, 205, kBorder);
    if (selected_) {
        const auto& fish = box[*selected_];
        const auto& species = fishing.database().at(fish.speciesId);
        fitText(species.name, {316, 106, 166, 23}, 17, rarityColor(species.rarity));
        text(TextFormat("%s  /  %.2f kg", rarityName(species.rarity), fish.weight), 316, 131, 12, kMuted);
        text(TextFormat("$%lld%s", static_cast<long long>(player.salePrice(fish)), fish.locked ? "   /   Protected" : "   sell value"), 316, 147, 12, fish.locked ? kGold : kMint);
        if (button({316, 164, 166, 22}, "Sell fish", !fish.locked, false, 3)) action = {UIActionType::SellStored, *selected_};
        if (button({316, 189, 166, 22}, fish.locked ? "Unlock fish" : "Lock fish", true, fish.locked, fish.locked ? 6 : 5)) action = {UIActionType::ToggleLock, *selected_};
    } else {
        DrawCircle(398, 135, 17, kRaised);
        fishIcon({0, 0.7f, 0}, fishing.database().at(0), {385, 122, 26, 26});
        text("Select a fish", 352, 159, 14, kText);
        text("View, sell or protect a catch", 316, 180, 12, kMuted);
    }
    const auto value = player.boxValue(true);
    if (button({28, 196, 150, 18}, "Sell all  " + moneyLabel(value), value > 0, false, 4)) action.type = UIActionType::SellAll;
    if (CheckCollisionPointRec(input_.mouse, {28, 196, 150, 18})) tooltip_ = "Sell all unlocked fish. Protected fish stay in your box.";
    text(box.empty() ? "Ready for a catch" : TextFormat("%d protected", lockedCount), 187, 198, 11, kMuted);
    return action;
}
void UI::collectionPanel(const FishingSystem& fishing, const Player& player) {
    int discovered = 0;
    for (std::size_t id = 0; id < fishing.database().all().size(); ++id) if (player.progress(static_cast<int>(id)).discovered) ++discovered;
    text("Field notes", 28, 82, 16, kText);
    text(TextFormat("%d / %d discovered", discovered, static_cast<int>(fishing.database().all().size())), 128, 85, 12, kMint);
    text("BEST WEIGHT", 387, 85, 11, kMuted);
    constexpr int visibleRows = 6;
    const int total = static_cast<int>(fishing.database().all().size());
    if (CheckCollisionPointRec(input_.mouse, {16,76,478,142}) && input_.wheel != 0)
        collectionScroll_ -= input_.wheel > 0 ? 1 : -1;
    collectionScroll_ = std::clamp(collectionScroll_, 0, std::max(0, total - visibleRows));
    if (total > visibleRows) {
        DrawRectangle(487,106,3,108,kRaised);
        const float thumb = 108.0f * visibleRows / total;
        DrawRectangleRec({487,106 + (108 - thumb) * collectionScroll_ / (total - visibleRows),3,thumb}, kMint);
    }
    for (int id = collectionScroll_; id < std::min(total, collectionScroll_ + visibleRows); ++id) {
        const auto entry = player.progress(static_cast<int>(id));
        const auto& species = fishing.database().at(static_cast<int>(id));
        const float y = 106 + static_cast<float>(id - collectionScroll_) * 18;
        if (id % 2 == 0) DrawRectangleRounded({25, y - 1, 460, 18}, 0.2f, 4, kRaised);
        if (entry.discovered) fishIcon({static_cast<int>(id), entry.recordWeight, 0}, species, {29, y - 4, 26, 24});
        else skin_.draw(UISkin::Unknown, 0, {34, y - 2, 16, 16});
        text(species.name.c_str(), 65, y, 13, entry.discovered ? kText : kMuted);
        text(rarityName(species.rarity), 260, y, 12, entry.discovered ? rarityColor(species.rarity) : kMuted);
        text(entry.discovered ? TextFormat("%.2f kg", entry.recordWeight) : "--", 418, y, 13, entry.discovered ? kMint : kMuted);
    }
}
void UI::catchPanel(const FishingSystem& fishing, const Player& player, bool autoSellCommon) {
    const auto& result = fishing.catchResult();
    const auto state = fishing.state();
    if ((state == FishingState::Caught || state == FishingState::Escaped) && result) {
        const bool caught = state == FishingState::Caught;
        const auto& species = fishing.database().at(result->speciesId);
        const int badge = !caught ? 3 : milestone_.newSpecies ? 0 : milestone_.newRecord ? 1 : player.boxFull() ? 2 : -1;
        if (badge >= 0) skin_.draw(UISkin::Badges, badge, {28, 82, 16, 16});
        text(caught ? (milestone_.newSpecies ? "NEW DISCOVERY" : milestone_.newRecord ? "PERSONAL BEST" : "FRESH CATCH") : "IT GOT AWAY!", badge >= 0 ? 50 : 28, 85, 12, caught ? kGold : kAlert);
        DrawCircle(54, 135, 23, kRaised);
        fishIcon(*result, species, {41, 122, 26, 26});
        text(species.name.c_str(), 91, 110, 24, caught ? rarityColor(species.rarity) : kText);
        text(TextFormat("%.2f kg   /   %s   /   $%lld", result->weight, rarityName(species.rarity), static_cast<long long>(player.salePrice(*result))), 92, 143, 14, kMuted);
        std::string status;
        if (!caught) status = TextFormat("Catch chance was %.1f%% at %02d:00. Try another cast.", fishing.lastAttemptChance()*100, fishing.lastAttemptHour());
        else if (autoSellCommon && species.rarity == FishRarity::Common) status = "Common catch - selling automatically.";
        else if (player.boxFull()) status = "Fish Box full. Sell a stored fish to keep fishing.";
        else status = TextFormat("Heading to your Fish Box  /  Slot %d of %d", static_cast<int>(player.fishBox().size()) + 1, player.capacity());
        fitText(status, {28, 172, 452, 20}, 14, caught && player.boxFull() ? kAlert : kMuted);
        track({28, 202, 454, 6}, fishing.progress(), caught ? kMint : kAlert);
    } else {
        text("SLOW DOWN. CAST A LINE.", 28, 86, 11, kMint);
        text("A quiet moment by the water.", 28, 107, 24, kText);
        text("Your next little discovery is on its way.", 28, 139, 14, kMuted);
        text(TextFormat("ROD %d   /   %.1fs avg. wait   /   %.1f%% catch", player.rodLevel(), 8.5f * FishingSystem::waitMultiplier(player.rodLevel())*(1-player.stats().waitReduction),
             player.stats().successChance(FishingSystem::successChance(localTime_.hour))*100), 28, 170, 13, kMint);
        fitText(autoSellCommon ? "Common fish sell automatically. Other catches go to your box." : "Every catch goes straight into your Fish Box.", {28, 195, 452, 16}, 12, kMuted);
    }
}

UIAction UI::draw(const FishingSystem& fishing, const Player& player, const std::optional<FishInstance>& lastCatch,
                  bool autoSellCommon, bool topmost, UIInput input) {
    initFont();
    skin_.load();
    skin_.setScale(scale());
    background_.update(localTime_.hour, frameDt_, preferences_.animateBackground);
    sprites_.setEffects(preferences_.waterEffects);
    sprites_.update(fishing, fishing.catchSettled() && player.boxFull() && !fishing.shouldAutoSell(autoSellCommon), localTime_.hour, frameDt_);
    frameDt_ = 0;
    input_ = input;
    tooltip_.clear();
    hovered_.reset();
    if (statsOpen_) {
        BeginMode2D(camera());
        const auto action = statsPanel(fishing, player);
        EndMode2D();
        return action;
    }
    if (onboardingOpen_) {
        UIAction action;
        const int sceneTop = height() - kCompactHeight;
        ClearBackground(kBackground);
        BeginMode2D(camera({0, static_cast<float>(sceneTop)}));
        scene(fishing, player, autoSellCommon);
        skin_.draw(UISkin::Status, 0, {8, kSceneHeight + 4, 496, 24});
        text("Welcome to Taskbar Fishing", 19, kSceneHeight + 8, 13, kMuted);
        EndMode2D();
        BeginMode2D(camera());
        action = welcomePanel(autoSellCommon, topmost);
        DrawRectangleLinesEx({0.5f, 0.5f, kWidth - 1, static_cast<float>(sceneTop + kCompactHeight - 1)}, 1, kBorder);
        if (sceneTop > 0) DrawLine(0, sceneTop, kWidth, sceneTop, kBorder);
        if (!tooltip_.empty() && preferences_.showTooltips) {
            card({8, static_cast<float>(sceneTop + kSceneHeight + 4), 496, 23}, kBackground);
            fitText(tooltip_, {16, static_cast<float>(sceneTop + kSceneHeight + 5), 480, 21}, 13, kText);
        }
        EndMode2D();
        if (input.released) pressedButton_.reset();
        return action;
    }
    if (dashboardOpen()) return dashboard(fishing, player, lastCatch, autoSellCommon);
    UIAction action;
    // Keep this frame's geometry stable when a button changes the next window size.
    const int sceneTop = height() - kCompactHeight;
    ClearBackground(kBackground);
    BeginMode2D(camera({0, static_cast<float>(sceneTop)}));
    scene(fishing, player, autoSellCommon);
    skin_.draw(UISkin::Status, 0, {8, kSceneHeight + 4, 496, 24});
    if (lastCatch) {
        fishIcon(*lastCatch, fishing.database().at(lastCatch->speciesId), {12, kSceneHeight + 3, 26, 24});
        const std::string label = TextFormat("Last catch  /  %s  /  %.2f kg", fishing.database().at(lastCatch->speciesId).name.c_str(), lastCatch->weight);
        fitText(label, {44, kSceneHeight + 5, 366, 21}, 13, kText);
        const std::string price = TextFormat("$%lld", static_cast<long long>(player.salePrice(*lastCatch)));
        text(price.c_str(), 488 - textWidth(price.c_str(), 13), kSceneHeight + 8, 13, kGold);
    } else text("Settle in. Your first catch is on its way...", 19, kSceneHeight + 8, 13, kMuted);
    EndMode2D();
    BeginMode2D(camera());
    if (extrasOpen_) {
        skin_.draw(UISkin::Icons, 3, {17, 14, 16, 16});
        text(moneyLabel(player.money()).c_str(), 41, 10, 22, kGold);
        DrawLine(151, 11, 151, 31, kBorder);
        text(TextFormat("Rod Lv.%d", player.rodLevel()), 165, 15, 14, kText);
        text(TextFormat("Fish Box  %d / %d", static_cast<int>(player.fishBox().size()), player.capacity()), 276, 15, 14, player.boxFull() ? kAlert : kMuted);
        text(TextFormat("Lv.%d", player.boxLevel()), 460, 16, 12, kMint);
        if (button({16, 40, 152, 25}, "Fish Box", true, boxOpen_, 0, true)) { boxOpen_ = true; collectionOpen_ = false; }
        if (button({178, 40, 152, 25}, "Collection", true, collectionOpen_, 1, true)) { collectionOpen_ = true; boxOpen_ = false; }
        if (button({340, 40, 154, 25}, "Catch", true, !boxOpen_ && !collectionOpen_, 2, true)) { boxOpen_ = collectionOpen_ = false; }
        card({16, 76, 478, 142});
        if (collectionOpen_) collectionPanel(fishing, player);
        else if (boxOpen_) action = fishBoxPanel(fishing, player);
        else catchPanel(fishing, player, autoSellCommon);
        if (button({16, 223, 478, 24}, "Equipment & upgrades", true, false, 7)) toggleStats();
    } else if (settingsOpen_) {
        action = settingsPanel(autoSellCommon,topmost);
    }
    const Rectangle extrasButton{452, static_cast<float>(sceneTop + 4), 24, 24};
    if (button({384, static_cast<float>(sceneTop+4), 62, 24}, "", true, false, 7)) toggleStats();
    if (CheckCollisionPointRec(input.mouse, {384, static_cast<float>(sceneTop+4), 62, 24})) tooltip_ = "Equipment & upgrades";
    const Rectangle settingsButton{480, static_cast<float>(sceneTop + 4), 24, 24};
    if (button(extrasButton, "", true, extrasOpen_, extrasOpen_ ? 10 : 9)) toggleExtras();
    if (button(settingsButton, "", true, settingsOpen_, 8)) toggleSettings();
    if (CheckCollisionPointRec(input.mouse, extrasButton)) tooltip_ = "Fish Box, collection & upgrades";
    if (CheckCollisionPointRec(input.mouse, settingsButton)) tooltip_ = "Settings";
    DrawRectangleLinesEx({0.5f, 0.5f, kWidth - 1, static_cast<float>(sceneTop + kCompactHeight - 1)}, 1, kBorder);
    if (sceneTop > 0) DrawLine(0, sceneTop, kWidth, sceneTop, kBorder);
    if (!tooltip_.empty() && preferences_.showTooltips) {
        card({8, static_cast<float>(sceneTop + kSceneHeight + 4), 496, 23}, kBackground);
        fitText(tooltip_, {16, static_cast<float>(sceneTop + kSceneHeight + 5), 480, 21}, 13, kText);
    }
    if (hovered_ && preferences_.showTooltips && boxOpen_ && *hovered_ < player.fishBox().size()) {
        const auto& fish = player.fishBox()[*hovered_];
        itemTooltip(fish, fishing.database().at(fish.speciesId), player);
    }
    EndMode2D();
    if (input.released) pressedButton_.reset();
    return action;
}
