#pragma once
#include "AssetTexture.h"
#include "AssetCatalog.h"
#include <algorithm>
#include <array>
#include <cmath>

// UI states and nine-slice borders are read from the same manifest as the scene.
class UISkin {
public:
    enum Asset { Panel, Status, Button, Tab, Slot, Overlay, Toggle, Progress, Icons, Badges, Unknown, Count };
    void load() {
        if (loaded_) return;
        for (int i = 0; i < Count; ++i) {
            textures_[i] = loadAssetTexture(AssetCatalog::get().sprite(names_[i]).texture);
            if (textures_[i].id) SetTextureFilter(textures_[i], TEXTURE_FILTER_POINT);
        }
        loaded_ = true;
    }
    void release() {
        for (auto& texture : textures_) {
            if (texture.id && IsWindowReady()) UnloadTexture(texture);
            texture = {};
        }
        loaded_ = false;
    }
    bool draw(Asset asset, int state, Rectangle dest, Color tint = WHITE) const {
        const auto& spec = AssetCatalog::get().sprite(names_[asset]);
        const auto texture = textures_[asset];
        if (!texture.id || state < 0 || static_cast<std::size_t>(state) >= spec.frames.size()) return false;
        const auto source = spec.frames[state].source;
        if (spec.left || spec.top || spec.right || spec.bottom) {
            NPatchInfo patch{source, spec.left, spec.top, spec.right, spec.bottom, NPATCH_NINE_PATCH};
            DrawTextureNPatch(texture, patch, dest, {0, 0}, 0, tint);
        } else DrawTexturePro(texture, source, dest, {0, 0}, 0, tint);
        return true;
    }
    void progress(Rectangle rect, float value, int state) const {
        draw(Progress, 0, rect);
        // Clip the completed strip so the caps stay the same size at every value.
        const float width = std::floor(rect.width * std::clamp(value, 0.0f, 1.0f));
        if (width <= 0) return;
        // Scissor rectangles are window pixels, so apply the UI scale the camera uses.
        BeginScissorMode(static_cast<int>(rect.x * scale_), static_cast<int>(rect.y * scale_),
                         static_cast<int>(std::ceil(width * scale_)), static_cast<int>(std::ceil(rect.height * scale_)));
        draw(Progress, state, rect);
        EndScissorMode();
    }
    void setScale(float scale) { scale_ = scale; }
private:
    float scale_ = 1;
    static constexpr std::array<const char*, Count> names_{
        "ui_panel", "ui_status_strip", "ui_button", "ui_tab", "ui_slot", "ui_slot_overlay",
        "ui_toggle", "ui_progress", "ui_icons", "ui_badges", "ui_unknown_fish"
    };
    std::array<Texture2D, Count> textures_{};
    bool loaded_ = false;
};
