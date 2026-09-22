#pragma once
#include <array>
#include <map>
#include <optional>
#include <string>
#include <vector>
#include <raylib.h>

struct SpriteFrame {
    Rectangle source{};
    double startMs = 0, durationMs = 0;
    Vector2 grip{}, tip{};
    std::optional<Vector2> hook;
    float rodAngle = 0;
};

struct SpriteDefinition {
    std::string texture, onComplete;
    std::array<std::string, 24> hourlyTextures{};
    Vector2 anchor{};
    bool loop = false, containsBobber = false;
    double durationMs = 0;
    int left = 0, top = 0, right = 0, bottom = 0;
    std::vector<SpriteFrame> frames;
    int frameAt(double elapsedMs) const;
};

struct SpriteLighting {
    float gain = 1, mix = 0;
    Color tint{255,255,255,255};
    Color apply(Color original) const;
};

class AssetCatalog {
public:
    static const AssetCatalog& get();
    const SpriteDefinition& sprite(const std::string& id) const { return sprites_.at(id); }
    const SpriteDefinition& background() const { return background_; }
    const SpriteLighting& lighting(int hour) const { return lighting_.at(hour); }
    Vector2 seat() const { return seat_; }
    Vector2 sceneSize() const { return sceneSize_; }
    double castEffectOffsetMs() const { return castEffectOffsetMs_; }
    std::string fishId(const std::string& name) const {
        const auto found = fishIds_.find(name);
        return found == fishIds_.end() ? std::string{} : found->second;
    }
private:
    AssetCatalog();
    std::map<std::string, SpriteDefinition> sprites_;
    std::map<std::string, std::string> fishIds_;
    SpriteDefinition background_;
    std::array<SpriteLighting, 24> lighting_{};
    Vector2 seat_{}, sceneSize_{};
    double castEffectOffsetMs_ = 320;
};
