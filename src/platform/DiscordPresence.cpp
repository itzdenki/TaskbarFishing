#include "DiscordPresence.h"
#include "EmbeddedAssets.h"
#include <sstream>
#include "fishing/FishingSystem.h"
#include "player/Player.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>
#include <nlohmann/json.hpp>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#ifdef FISHING_DISCORD_TEST_TRANSPORT
#define FISHING_DISCORD_PIPE_PREFIX L"\\\\?\\pipe\\taskbar-fishing-presence-test-"
#else
#define FISHING_DISCORD_PIPE_PREFIX L"\\\\?\\pipe\\discord-ipc-"
#endif
#endif

using Clock = std::chrono::steady_clock;
using namespace std::chrono_literals;
using Json = nlohmann::json;

bool DiscordPresence::validApplicationId(std::string_view id) {
    return id.size() >= 17 && id.size() <= 20 && id.front() != '0' &&
        std::all_of(id.begin(), id.end(), [](char c) { return c >= '0' && c <= '9'; });
}

DiscordArtwork DiscordPresence::readArtwork(const std::filesystem::path& file) {
    std::istringstream input(assets::readText(file));
    const auto config = Json::parse(input, nullptr, false);
    if (!config.is_object()) return {};
    auto field = [&](const char* key, std::size_t limit) -> std::string {
        const auto value = config.find(key);
        if (value == config.end() || !value->is_string()) return {};
        const auto text = value->get<std::string>();
        if (text.size() > limit || std::any_of(text.begin(), text.end(), [](unsigned char c) { return c < 32; })) return {};
        return text;
    };
    DiscordArtwork artwork{field("large_image", 256), field("large_text", 128)};
    // Discord accepts uploaded asset keys or publicly accessible HTTPS images, never local paths.
    if (!artwork.largeImage.starts_with("https://") &&
        artwork.largeImage.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789_-") != std::string::npos)
        artwork.largeImage.clear();
    return artwork;
}

DiscordActivity DiscordPresence::activityFor(const FishingSystem& fishing, const Player& player, bool autoSellCommon) {
    DiscordActivity activity;
    switch (fishing.state()) {
    case FishingState::Idle: activity.details = "Relaxing by the water"; break;
    case FishingState::Casting: activity.details = "Casting a line"; break;
    case FishingState::Waiting: activity.details = "Waiting for a bite"; break;
    case FishingState::FishBiting: activity.details = "Got a bite!"; break;
    case FishingState::Reeling: activity.details = "Reeling in a fish"; break;
    case FishingState::Escaped: activity.details = "One got away!"; break;
    case FishingState::Caught:
        if (fishing.catchSettled() && player.boxFull() && !fishing.shouldAutoSell(autoSellCommon))
            activity.details = "Fish Box full - taking a break";
        else if (fishing.catchResult())
            activity.details = "Caught " + fishing.database().at(fishing.catchResult()->speciesId).name + "!";
        else activity.details = "Landed a fish!";
        break;
    }
    activity.state = (player.name().empty() ? "" : player.name() + " | ") + "Rod Lv." + std::to_string(player.rodLevel()) + " | Fish Box " +
        std::to_string(player.fishBox().size()) + "/" + std::to_string(player.capacity());
    return activity;
}

struct DiscordPresence::Impl {
    std::mutex mutex;
    DiscordActivity latest;
    bool hasActivity = false;
    const std::string applicationId;
    const DiscordArtwork artwork;
    const std::int64_t started = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
#ifdef _WIN32
    // Declared last: joins before the state used by the worker is destroyed.
    std::jthread worker;
#endif

    explicit Impl(std::string id, DiscordArtwork art) : applicationId(std::move(id)), artwork(std::move(art)) {
#ifdef _WIN32
        worker = std::jthread([this](std::stop_token stop) {
            try { run(stop); }
            catch (const std::exception& error) { std::clog << "Discord Presence stopped: " << error.what() << '\n'; }
        });
#endif
    }

#ifdef _WIN32
    struct Pipe {
        HANDLE handle = INVALID_HANDLE_VALUE;
        ~Pipe() { if (handle != INVALID_HANDLE_VALUE) CloseHandle(handle); }
    };

    static bool writeFrame(HANDLE pipe, std::uint32_t opcode, const std::string& payload) {
        std::vector<unsigned char> bytes(8 + payload.size());
        for (unsigned i = 0; i < 4; ++i) {
            bytes[i] = static_cast<unsigned char>(opcode >> (8 * i));
            bytes[4 + i] = static_cast<unsigned char>(payload.size() >> (8 * i));
        }
        std::copy(payload.begin(), payload.end(), bytes.begin() + 8);
        OVERLAPPED operation{};
        operation.hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        if (!operation.hEvent) return false;
        DWORD written = 0;
        bool ok = WriteFile(pipe, bytes.data(), static_cast<DWORD>(bytes.size()), &written, &operation) != FALSE;
        if (!ok && GetLastError() == ERROR_IO_PENDING) {
            if (WaitForSingleObject(operation.hEvent, 200) == WAIT_OBJECT_0)
                ok = GetOverlappedResult(pipe, &operation, &written, FALSE) != FALSE;
            else {
                CancelIoEx(pipe, &operation);
                GetOverlappedResult(pipe, &operation, &written, TRUE);
            }
        }
        CloseHandle(operation.hEvent);
        return ok && written == bytes.size();
    }

    // Read only available bytes; retain partial frames between polls.
    static bool receive(HANDLE pipe, std::vector<unsigned char>& buffer) {
        DWORD available = 0;
        if (!PeekNamedPipe(pipe, nullptr, 0, nullptr, &available, nullptr)) return false;
        if (!available) return true;
        if (buffer.size() + available > 128 * 1024) return false;
        std::array<unsigned char, 16384> bytes{};
        DWORD count = 0;
        OVERLAPPED operation{};
        operation.hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        if (!operation.hEvent) return false;
        bool ok = ReadFile(pipe, bytes.data(), (std::min)(available, static_cast<DWORD>(bytes.size())), &count, &operation) != FALSE;
        if (!ok && GetLastError() == ERROR_IO_PENDING) {
            if (WaitForSingleObject(operation.hEvent, 200) == WAIT_OBJECT_0)
                ok = GetOverlappedResult(pipe, &operation, &count, FALSE) != FALSE;
            else {
                CancelIoEx(pipe, &operation);
                GetOverlappedResult(pipe, &operation, &count, TRUE);
            }
        }
        CloseHandle(operation.hEvent);
        if (ok) buffer.insert(buffer.end(), bytes.begin(), bytes.begin() + count);
        return ok;
    }

    void run(std::stop_token stop) {
        std::uint64_t nonce = 0;
        auto retryAt = Clock::now();
        while (!stop.stop_requested()) {
            if (Clock::now() < retryAt) { std::this_thread::sleep_for(50ms); continue; }
            Pipe pipe;
            for (int index = 0; index < 10 && pipe.handle == INVALID_HANDLE_VALUE; ++index) {
                const auto name = FISHING_DISCORD_PIPE_PREFIX + std::to_wstring(index);
                pipe.handle = CreateFileW(name.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr,
                    OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr);
            }
            retryAt = Clock::now() + 5s;
            if (pipe.handle == INVALID_HANDLE_VALUE) continue;
            if (!writeFrame(pipe.handle, 0, Json{{"v", 1}, {"client_id", applicationId}}.dump())) continue;
            bool ready = false, connected = true;
            std::string pendingNonce;
            auto deadline = Clock::now() + 10s;
            auto nextUpdate = Clock::now();
            DiscordActivity sent;
            bool hasSent = false;
            std::vector<unsigned char> buffer;
            while (!stop.stop_requested() && connected) {
                if (!receive(pipe.handle, buffer)) break;
                while (buffer.size() >= 8) {
                    auto word = [&](std::size_t offset) {
                        std::uint32_t value = 0;
                        for (unsigned i = 0; i < 4; ++i) value |= std::uint32_t(buffer[offset + i]) << (8 * i);
                        return value;
                    };
                    const auto opcode = word(0), length = word(4);
                    if (length > 64 * 1024) { connected = false; break; }
                    if (buffer.size() < 8 + length) break;
                    const std::string payload(buffer.begin() + 8, buffer.begin() + 8 + length);
                    buffer.erase(buffer.begin(), buffer.begin() + 8 + length);
                    if (opcode == 2) { connected = false; break; }
                    if (opcode == 3) { connected = writeFrame(pipe.handle, 4, payload); if (!connected) break; }
                    else if (opcode == 1) {
                        const auto message = Json::parse(payload, nullptr, false);
                        if (!message.is_object()) { connected = false; break; }
                        if (message.contains("evt") && message["evt"] == "ERROR") {
                            std::clog << "Discord rejected Presence; check the Application ID.\n";
                            connected = false; break;
                        }
                        if (message.contains("evt") && message["evt"] == "READY") ready = true;
                        if (!pendingNonce.empty() && message.contains("nonce") && message["nonce"] == pendingNonce)
                            pendingNonce.clear();
                    }
                }
                if (!connected || ((!ready || !pendingNonce.empty()) && Clock::now() >= deadline)) break;
                if (ready && pendingNonce.empty() && Clock::now() >= nextUpdate) {
                    DiscordActivity activity;
                    bool available;
                    { std::lock_guard lock(mutex); activity = latest; available = hasActivity; }
                    if (available && (!hasSent || !(activity == sent))) {
                        pendingNonce = std::to_string(++nonce);
                        Json message{{"cmd", "SET_ACTIVITY"}, {"nonce", pendingNonce},
                            {"args", {{"pid", GetCurrentProcessId()}, {"activity", {
                                {"details", activity.details}, {"state", activity.state},
                                {"timestamps", {{"start", started}}}, {"instance", false}}}}}};
                        if (!artwork.largeImage.empty()) {
                            auto& assets = message["args"]["activity"]["assets"];
                            assets["large_image"] = artwork.largeImage;
                            if (!artwork.largeText.empty()) assets["large_text"] = artwork.largeText;
                        }
                        if (!writeFrame(pipe.handle, 1, message.dump())) break;
                        sent = std::move(activity);
                        hasSent = true;
                        nextUpdate = Clock::now() + 15s;
                        deadline = Clock::now() + 10s;
                    }
                }
                std::this_thread::sleep_for(50ms);
            }
            if (stop.stop_requested() && ready) {
                writeFrame(pipe.handle, 1, Json{{"cmd", "SET_ACTIVITY"}, {"nonce", std::to_string(++nonce)},
                    {"args", {{"pid", GetCurrentProcessId()}, {"activity", nullptr}}}}.dump());
            }
            retryAt = Clock::now() + 5s;
        }
    }
#endif
};

DiscordPresence::DiscordPresence(std::string applicationId, DiscordArtwork artwork) {
    if (validApplicationId(applicationId)) {
        try { impl_ = std::make_unique<Impl>(std::move(applicationId), std::move(artwork)); }
        catch (const std::system_error& error) {
            std::clog << "Discord Presence unavailable: " << error.what() << '\n';
        }
    }
}
DiscordPresence::~DiscordPresence() = default;
void DiscordPresence::update(const DiscordActivity& activity) {
    if (!impl_) return;
    std::lock_guard lock(impl_->mutex);
    impl_->latest = activity;
    impl_->hasActivity = true;
}
