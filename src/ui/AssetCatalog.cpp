#include "AssetCatalog.h"
#include "platform/EmbeddedAssets.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <nlohmann/json.hpp>

namespace {
using Json = nlohmann::json;
Json readManifest(const std::string& name) {
    const auto bytes = assets::embedded(name);
    if (!bytes.empty()) return Json::parse(bytes.begin(), bytes.end());
    std::ifstream stream(assets::applicationDirectory(GetApplicationDirectory()) / name, std::ios::binary);
    return Json::parse(stream);
}
Vector2 point(const Json& value) { return {value.at("x").get<float>(), value.at("y").get<float>()}; }
Rectangle rectangle(const Json& value) {
    return {value.at("x").get<float>(), value.at("y").get<float>(), value.at("width").get<float>(), value.at("height").get<float>()};
}
std::string path(const Json& value) { return "assets/" + value.get<std::string>(); }
SpriteDefinition definition(const Json& value, Vector2 anchor = {}) {
    SpriteDefinition result;
    if (value.contains("texture")) result.texture = path(value.at("texture"));
    result.anchor = value.contains("anchor") ? point(value.at("anchor")) : anchor;
    result.loop = value.value("loop", false);
    result.onComplete = value.value("on_complete", "hold_last");
    result.containsBobber = value.value("contains_bobber", false);
    result.durationMs = value.value("duration_ms", value.value("loop_duration_ms", 0.0));
    if (value.contains("nine_slice") && !value.at("nine_slice").is_null()) {
        const auto& slice = value.at("nine_slice");
        result.left = slice.at("left"); result.right = slice.at("right");
        result.top = slice.at("top"); result.bottom = slice.at("bottom");
    }
    const auto& frames = value.contains("frames") ? value.at("frames") : value.at("states");
    double elapsed = 0;
    for (const auto& item : frames) {
        SpriteFrame frame;
        frame.source = rectangle(item.at("source"));
        frame.startMs = item.value("start_ms", elapsed);
        frame.durationMs = item.value("duration_ms", 0.0);
        if (item.contains("rod_grip")) frame.grip = point(item.at("rod_grip"));
        if (item.contains("tip")) frame.tip = point(item.at("tip"));
        if (item.contains("hook_anchor")) frame.hook = point(item.at("hook_anchor"));
        frame.rodAngle = item.value("rod_angle_deg", 0.0f);
        if (frame.source.width <= 0 || frame.source.height <= 0 || frame.durationMs < 0)
            throw std::runtime_error("Invalid sprite rectangle or timing");
        result.frames.push_back(frame);
        elapsed += frame.durationMs;
    }
    if (result.frames.empty()) throw std::runtime_error("Sprite has no frames");
    if (result.durationMs > 0 && std::abs(elapsed - result.durationMs) > 0.01)
        throw std::runtime_error("Sprite duration does not match frame timings");
    if (value.contains("hourly_textures")) for (const auto& hour : value.at("hourly_textures"))
        result.hourlyTextures.at(hour.at("hour").get<int>()) = path(hour.at("texture"));
    return result;
}
}

int SpriteDefinition::frameAt(double elapsedMs) const {
    if (frames.empty() || !std::isfinite(elapsedMs) || elapsedMs < 0) return -1;
    if (durationMs <= 0) return 0;
    if (loop) elapsedMs = std::fmod(elapsedMs, durationMs);
    else if (elapsedMs >= durationMs) return onComplete == "hide" ? -1 : static_cast<int>(frames.size()) - 1;
    for (std::size_t i = 0; i < frames.size(); ++i)
        if (elapsedMs < frames[i].startMs + frames[i].durationMs) return static_cast<int>(i);
    return static_cast<int>(frames.size()) - 1;
}

Color SpriteLighting::apply(Color original) const {
    const auto channel = [&](unsigned char source, unsigned char color) {
        return static_cast<unsigned char>(std::clamp(std::lround(source * gain * (1 - mix) + color * mix), 0L, 255L));
    };
    return {channel(original.r, tint.r), channel(original.g, tint.g), channel(original.b, tint.b), original.a};
}

const AssetCatalog& AssetCatalog::get() {
    static const AssetCatalog catalog;
    return catalog;
}

AssetCatalog::AssetCatalog() {
    const auto root = readManifest("assets/assets.json");
    if (root.at("schema_version") != 1) throw std::runtime_error("Unsupported assets.json schema");
    sceneSize_ = {root.at("scene").at("width").get<float>(), root.at("scene").at("height").get<float>()};
    seat_ = point(root.at("integration").at("seat_example"));
    for (const auto& event : root.at("integration").at("events"))
        if (event.at("name") == "cast_start") castEffectOffsetMs_ = event.at("offset_ms");
    background_ = definition(root.at("background"));
    for (const auto& hour : root.at("background").at("hours"))
        background_.hourlyTextures.at(hour.at("hour").get<int>()) = path(hour.at("texture"));
    const auto anchor = point(root.at("player").at("anchor"));
    for (const auto& item : root.at("player").at("animations")) sprites_.emplace(item.at("id").get<std::string>(), definition(item, anchor));
    for (const auto& item : root.at("effects")) sprites_.emplace(item.at("id").get<std::string>(), definition(item));
    for (const auto& item : root.at("ui").at("components")) sprites_.emplace(item.at("id").get<std::string>(), definition(item));
    const auto& rod = root.at("equipment").at("rod");
    sprites_.emplace(rod.at("id").get<std::string>(), definition(rod));
    for (const auto& hour : root.at("lighting").at("hours")) {
        auto& light = lighting_.at(hour.at("hour").get<int>());
        light.gain = hour.at("gain"); light.mix = hour.at("mix");
        const auto& rgb = hour.at("tint_rgb");
        light.tint = {rgb.at(0).get<unsigned char>(), rgb.at(1).get<unsigned char>(), rgb.at(2).get<unsigned char>(), 255};
    }
    const auto fish = readManifest("assets/fish/fish.json");
    for (const auto& item : fish.at("species")) {
        const auto id = item.at("id").get<std::string>();
        auto animation = item.at("animation");
        animation["texture"] = item.at("texture");
        animation["anchor"] = item.at("hook_anchor");
        sprites_.emplace("fish/" + id, definition(animation));
        SpriteDefinition icon;
        icon.texture = path(item.at("icon"));
        SpriteFrame frame;
        frame.source = rectangle(item.at("icon_source"));
        icon.frames.push_back(frame);
        sprites_.emplace("fish_icon/" + id, std::move(icon));
        fishIds_.emplace(item.at("name").get<std::string>(), id);
    }
    fishIds_.emplace("Carp", "common_carp");
    fishIds_.emplace("Bass", "largemouth_bass");
    const auto special = readManifest("assets/special/special.json");
    for (const auto& item : special.at("items")) {
        const auto id = item.at("id").get<std::string>();
        for (const auto& animation : item.at("animations")) {
            const std::string prefix = animation.at("id") == "caught" ? "fish/" : "fish_float/";
            sprites_.emplace(prefix + id, definition(animation, point(item.at("anchor"))));
        }
        SpriteDefinition icon;
        icon.texture = path(item.at("icon"));
        SpriteFrame frame;
        frame.source = rectangle(item.at("icon_source"));
        icon.frames.push_back(frame);
        sprites_.emplace("fish_icon/" + id, std::move(icon));
        fishIds_.emplace(item.at("name").get<std::string>(), id);
    }
}
