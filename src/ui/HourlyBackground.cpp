#include "HourlyBackground.h"
#include "AssetTexture.h"
#include "AssetCatalog.h"
#include <algorithm>
#include <cmath>
#include <string>

void HourlyBackground::release() {
    for (auto& texture : textures_) {
        if (texture.id && IsWindowReady()) UnloadTexture(texture);
        texture = {};
    }
    missing_ = {};
    hour_ = previous_ = -1;
    fade_ = 1;
    elapsedMs_ = 0;
}

void HourlyBackground::update(int hour, float dt, bool animate) {
    const auto& atlas = AssetCatalog::get().background();
    if (animate && std::isfinite(dt) && dt > 0) elapsedMs_ = std::fmod(elapsedMs_ + dt * 1000.0, atlas.durationMs);
    hour = (hour % 24 + 24) % 24;
    if (hour != hour_) {
        // If a requested file was missing, keep the last valid scene as fallback.
        if (available()) previous_ = hour_;
        hour_ = hour;
        fade_ = previous_ >= 0 ? 0 : 1;
        for (int i = 0; i < 24; ++i) {
            if (i != hour && i != previous_ && textures_[i].id) {
                UnloadTexture(textures_[i]); textures_[i] = {};
            }
        }
        for (int i : {hour}) {
            if (textures_[i].id || missing_[i]) continue;
            textures_[i] = loadAssetTexture(atlas.hourlyTextures[i]);
            missing_[i] = textures_[i].id == 0;
            if (textures_[i].id) SetTextureFilter(textures_[i], TEXTURE_FILTER_POINT);
        }
    } else fade_ = std::min(1.0f, fade_ + std::max(0.0f, dt) / 0.8f);
    if (fade_ >= 1 && available() && previous_ >= 0 && previous_ != hour_) {
        UnloadTexture(textures_[previous_]); textures_[previous_] = {};
        previous_ = -1;
    }
}

int HourlyBackground::frameIndex() const { return AssetCatalog::get().background().frameAt(elapsedMs_); }

bool HourlyBackground::draw(Rectangle destination) const {
    const auto source = AssetCatalog::get().background().frames.at(frameIndex()).source;
    const auto drawTexture = [&](Texture2D texture, float opacity) {
        if (!texture.id) return;
        DrawTexturePro(texture, source,
                       destination, {0, 0}, 0, Fade(WHITE, opacity));
    };
    if (previous_ >= 0 && (!available() || fade_ < 1)) drawTexture(textures_[previous_], 1);
    if (available()) drawTexture(textures_[hour_], fade_);
    return available() || (previous_ >= 0 && textures_[previous_].id);
}
