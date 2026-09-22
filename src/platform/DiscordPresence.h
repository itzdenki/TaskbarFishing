#pragma once
#include <chrono>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>

class FishingSystem;
class Player;

struct DiscordActivity {
    std::string details;
    std::string state;
    bool operator==(const DiscordActivity&) const = default;
};

struct DiscordArtwork {
    std::string largeImage;
    std::string largeText;
};

// Local Discord IPC only; no account credentials or network requests.
class DiscordPresence {
public:
    explicit DiscordPresence(std::string applicationId, DiscordArtwork artwork = {});
    ~DiscordPresence();
    DiscordPresence(const DiscordPresence&) = delete;
    DiscordPresence& operator=(const DiscordPresence&) = delete;
    void update(const DiscordActivity& activity);
    static bool validApplicationId(std::string_view id);
    static DiscordArtwork readArtwork(const std::filesystem::path& file);
    static DiscordActivity activityFor(const FishingSystem& fishing, const Player& player, bool autoSellCommon);
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
