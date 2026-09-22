#include "FishingSprites.h"
#include "AssetTexture.h"
#include <algorithm>
#include <cmath>

namespace {
constexpr Vector2 kWater{316, 115}, kTakeoff{260, 115};
Vector2 add(Vector2 a, Vector2 b) { return {a.x + b.x, a.y + b.y}; }
Vector2 subtract(Vector2 a, Vector2 b) { return {a.x - b.x, a.y - b.y}; }
Vector2 lerp(Vector2 a, Vector2 b, float t) { return {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t}; }
Vector2 rotate(Vector2 value, float degrees) {
    const float angle = degrees * DEG2RAD;
    return {value.x * std::cos(angle) - value.y * std::sin(angle), value.x * std::sin(angle) + value.y * std::cos(angle)};
}
}

void FishingSprites::clearTextures() {
    for (const auto& [id, value] : textures_) if (value.id && IsWindowReady()) UnloadTexture(value);
    textures_.clear();
}
void FishingSprites::release() {
    clearTextures();
    hour_ = -1;
    previousState_.reset();
    castMs_ = impactMs_ = -1;
}

Texture2D FishingSprites::texture(const std::string& id, bool player) {
    const auto found = textures_.find(id);
    if (found != textures_.end()) return found->second;
    const auto& catalog = AssetCatalog::get();
    const auto& def = catalog.sprite(id);
    const bool baked = !def.hourlyTextures[hour_].empty();
    Image image = loadAssetImage(baked ? def.hourlyTextures[hour_] : def.texture);
    Texture2D value{};
    if (image.data) {
        // Player and fish sheets have no baked variants; preserve alpha while lighting them.
        if (player && !baked) {
            ImageFormat(&image, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
            auto* pixels = static_cast<Color*>(image.data);
            for (int i = 0; i < image.width * image.height; ++i) pixels[i] = catalog.lighting(hour_).apply(pixels[i]);
        }
        value = LoadTextureFromImage(image);
        UnloadImage(image);
        if (value.id) SetTextureFilter(value, TEXTURE_FILTER_POINT);
    }
    textures_.emplace(id, value);
    return value;
}

void FishingSprites::update(const FishingSystem& fishing, bool boxFull, int hour, float dt) {
    hour = (hour % 24 + 24) % 24;
    if (hour != hour_) { clearTextures(); hour_ = hour; }
    const double delta = std::isfinite(dt) ? std::max(0.0f, dt) * 1000.0 : 0;
    if (castMs_ >= 0) castMs_ += delta;
    if (impactMs_ >= 0) impactMs_ += delta;
    const auto state = fishing.state();
    const double elapsed = fishing.elapsed() * 1000.0;
    const auto& catalog = AssetCatalog::get();
    if (state == FishingState::Casting) {
        // Gameplay's cast duration may differ slightly from the authored 680 ms animation.
        animationMs_ = fishing.progress() * catalog.sprite("casting").durationMs;
        castMs_ = animationMs_ - catalog.castEffectOffsetMs();
        const auto& cast = catalog.sprite("casting");
        const auto& release = cast.frames.at(cast.frameAt(catalog.castEffectOffsetMs()));
        castOrigin_ = add(subtract(catalog.seat(), cast.anchor), release.grip);
    } else animationMs_ = elapsed;
    if ((!previousState_ || *previousState_ != state) && state == FishingState::Waiting) impactMs_ = elapsed;
    switch (state) {
    case FishingState::Idle: animation_ = "idle"; break;
    case FishingState::Casting: animation_ = "casting"; break;
    case FishingState::Waiting: animation_ = "idle"; break;
    case FishingState::FishBiting: animation_ = "bite"; break;
    case FishingState::Reeling: animation_ = "reeling"; break;
    case FishingState::Caught:
        animation_ = boxFull ? "box_full" : "caught";
        if (boxFull) animationMs_ = std::max(0.0, elapsed - FishingSystem::kCaughtDisplay * 1000.0);
        break;
    case FishingState::Escaped: animation_ = "escaped"; break;
    }
    const auto& def = catalog.sprite(animation_);
    if (!def.loop && def.onComplete == "idle" && animationMs_ >= def.durationMs) {
        animationMs_ -= def.durationMs;
        animation_ = "idle";
    }
    playerFrame_ = catalog.sprite(animation_).frameAt(animationMs_);
    previousState_ = state;
}

void FishingSprites::sprite(const std::string& id, double elapsedMs, Vector2 position) {
    if (!effects_ && id != "bobber_idle" && id != "fx_bobber_bite") return;
    const auto& def = AssetCatalog::get().sprite(id);
    const int index = def.frameAt(elapsedMs);
    if (index < 0) return;
    const auto value = texture(id);
    if (!value.id) return;
    const auto source = def.frames[index].source;
    DrawTexturePro(value, source, {std::round(position.x - def.anchor.x), std::round(position.y - def.anchor.y), source.width, source.height}, {0, 0}, 0, WHITE);
}

bool FishingSprites::drawIcon(const std::string& speciesName, Rectangle cell, float maximumScale) {
    const auto& catalog = AssetCatalog::get();
    const auto id = catalog.fishId(speciesName);
    if (id.empty()) return false;
    const auto key = "fish_icon/" + id;
    const auto value = texture(key);
    if (!value.id) return false;
    const auto source = catalog.sprite(key).frames.front().source;
    const float scale = std::min({maximumScale, cell.width / source.width, cell.height / source.height});
    const float w = source.width * scale, h = source.height * scale;
    DrawTexturePro(value, source, {std::floor(cell.x + (cell.width - w) / 2), std::floor(cell.y + (cell.height - h) / 2), w, h}, {0,0}, 0, WHITE);
    return true;
}

void FishingSprites::hooked(const std::string& key, double elapsedMs, Vector2 endpoint, float rotation) {
    const auto& def = AssetCatalog::get().sprite(key);
    const auto& frame = def.frames.at(def.frameAt(elapsedMs));
    const auto value = texture(key, true);
    const float scale = std::min(1.0f, 24.0f / frame.source.width);
    const auto hook = frame.hook.value_or(def.anchor);
    if (value.id) DrawTexturePro(value, frame.source,
        {endpoint.x, endpoint.y, frame.source.width * scale, frame.source.height * scale},
        {hook.x * scale, hook.y * scale}, rotation, WHITE);
}

void FishingSprites::draw(const FishingSystem& fishing, const std::function<void(Rectangle)>& drawFish) {
    const auto& catalog = AssetCatalog::get();
    const auto& player = catalog.sprite(animation_);
    const auto& frame = player.frames.at(playerFrame_);
    const Vector2 origin = subtract(catalog.seat(), player.anchor);
    const Vector2 grip = add(origin, frame.grip);
    const auto state = fishing.state();
    const double elapsed = fishing.elapsed() * 1000.0;
    const Vector2 contact = state == FishingState::Reeling ? lerp(kWater, kTakeoff, fishing.progress()) : kWater;
    const auto& rod = catalog.sprite("rod_basic");
    const int bend = state == FishingState::Reeling ? 2 : state == FishingState::FishBiting ? 1 : 0;
    const auto& rodFrame = rod.frames[bend];
    const Vector2 tip = add(grip, rotate(subtract(rodFrame.tip, rod.anchor), frame.rodAngle));
    Vector2 endpoint = contact;
    if (state == FishingState::Casting) {
        const float flight = std::clamp(static_cast<float>((animationMs_ - catalog.castEffectOffsetMs()) / (catalog.sprite("casting").durationMs - catalog.castEffectOffsetMs())), 0.0f, 1.0f);
        endpoint = lerp(tip, kWater, flight);
        endpoint.y -= std::sin(flight * PI) * 22;
    }
    if (state == FishingState::Caught) {
        endpoint = lerp(kTakeoff, {222, 77}, std::clamp(fishing.elapsed() / 0.6f, 0.0f, 1.0f));
    }
    if (state == FishingState::Idle || state == FishingState::Escaped) endpoint = add(tip, {5, 22});

    sprite("fx_water_impact", impactMs_, kWater);
    if (state == FishingState::Waiting) sprite("fx_wait_ripple", elapsed, contact);
    if (state == FishingState::Reeling) sprite("fx_reel_wake", elapsed, contact);
    if (state == FishingState::Caught) sprite("fx_lift_splash", elapsed, kTakeoff);
    if (state == FishingState::Escaped) sprite("fx_water_impact", elapsed, kTakeoff);

    const auto playerTexture = texture(animation_, true);
    if (playerTexture.id) DrawTexturePro(playerTexture, frame.source, {origin.x, origin.y, frame.source.width, frame.source.height}, {0, 0}, 0, WHITE);
    const auto rodTexture = texture("rod_basic");
    if (rodTexture.id) DrawTexturePro(rodTexture, rodFrame.source, {grip.x, grip.y, rodFrame.source.width, rodFrame.source.height}, rod.anchor, frame.rodAngle, WHITE);
    Vector2 line[]{tip, {(tip.x + endpoint.x) / 2, (tip.y + endpoint.y) / 2 + (state == FishingState::Reeling ? 1.0f : 8.0f)}, endpoint};
    DrawSplineBezierQuadratic(line, 3, 1, catalog.lighting(hour_).apply({231, 239, 233, 210}));

    if (state == FishingState::Caught && fishing.catchResult()) {
        const auto id = catalog.fishId(fishing.database().at(fishing.catchResult()->speciesId).name);
        if (!id.empty()) {
            // Fish sheets face right: rotate around the mouth so the tail hangs down.
            const bool special = fishing.database().at(fishing.catchResult()->speciesId).rarity == FishRarity::Special;
            hooked("fish/" + id, elapsed, endpoint, special ? 0.0f : -90.0f);
        } else drawFish({endpoint.x - 13, endpoint.y - 12, 26, 24});
        sprite("fx_lift_drops", elapsed, endpoint);
    } else if (state == FishingState::Reeling && fishing.catchResult() && fishing.database().at(fishing.catchResult()->speciesId).rarity == FishRarity::Special) {
        hooked("fish_float/" + catalog.fishId(fishing.database().at(fishing.catchResult()->speciesId).name), elapsed, endpoint);
    } else if (state == FishingState::FishBiting && catalog.sprite("fx_bobber_bite").containsBobber) {
        sprite("fx_bobber_bite", elapsed, contact);
    } else sprite("bobber_idle", elapsed, endpoint);
    sprite("fx_cast_swoosh", castMs_, castOrigin_);
    if (state == FishingState::FishBiting) sprite("fx_bite_alert", elapsed, {catalog.seat().x, origin.y + 4});
}
