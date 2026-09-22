#pragma once
#include "AssetCatalog.h"
#include "fishing/FishingSystem.h"
#include <functional>
#include <map>

class FishingSprites {
public:
    FishingSprites() = default;
    ~FishingSprites() { release(); }
    FishingSprites(const FishingSprites&) = delete;
    FishingSprites& operator=(const FishingSprites&) = delete;
    void release();
    void update(const FishingSystem& fishing, bool boxFull, int hour, float dt);
    void draw(const FishingSystem& fishing, const std::function<void(Rectangle)>& drawFish);
    bool drawIcon(const std::string& speciesName, Rectangle cell, float maximumScale = 1);
    void setEffects(bool enabled) { effects_=enabled; }
    const std::string& animation() const { return animation_; }
    int playerFrame() const { return playerFrame_; }
private:
    Texture2D texture(const std::string& id, bool player = false);
    void sprite(const std::string& id, double elapsedMs, Vector2 position);
    void clearTextures();
    void hooked(const std::string& key, double elapsedMs, Vector2 endpoint, float rotation = 0);
    std::map<std::string, Texture2D> textures_;
    int hour_ = -1, playerFrame_ = 0;
    std::optional<FishingState> previousState_;
    std::string animation_ = "idle";
    double animationMs_ = 0, castMs_ = -1, impactMs_ = -1;
    Vector2 castOrigin_{};
    bool effects_=true;
};
