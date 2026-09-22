#pragma once
#include "platform/EmbeddedAssets.h"
#include <raylib.h>
#include <string>

inline Image loadAssetImage(const std::string& name) {
    const auto bytes = assets::embedded(name);
    if (!bytes.empty()) {
        return LoadImageFromMemory(".png", bytes.data(), static_cast<int>(bytes.size()));
    }
    return LoadImage((std::string(GetApplicationDirectory()) + name).c_str());
}

inline Texture2D loadAssetTexture(const std::string& name) {
    Image image = loadAssetImage(name);
    if (!image.data) return {};
    const auto texture = LoadTextureFromImage(image);
    UnloadImage(image);
    return texture;
}
