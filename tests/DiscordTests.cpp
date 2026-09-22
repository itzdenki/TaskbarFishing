#include "platform/DiscordPresence.h"
#include "fishing/FishingSystem.h"
#include "player/Player.h"
#include <nlohmann/json.hpp>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <fstream>
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

using namespace std::chrono_literals;
using Json = nlohmann::json;
void check(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }

struct Server {
    HANDLE pipe = CreateNamedPipeW(L"\\\\?\\pipe\\taskbar-fishing-presence-test-0", PIPE_ACCESS_DUPLEX,
        PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT, 1, 65536, 65536, 0, nullptr);
    Server() { check(pipe != INVALID_HANDLE_VALUE, "create isolated test pipe"); }
    ~Server() { DisconnectNamedPipe(pipe); CloseHandle(pipe); }
    void connect() { check(ConnectNamedPipe(pipe, nullptr) || GetLastError() == ERROR_PIPE_CONNECTED, "connect"); }
    void bytes(void* data, DWORD size) {
        DWORD total = 0;
        while (total < size) {
            DWORD count = 0;
            check(ReadFile(pipe, static_cast<char*>(data) + total, size - total, &count, nullptr) && count, "read");
            total += count;
        }
    }
    std::pair<unsigned, std::string> read() {
        unsigned header[2]; bytes(header, sizeof(header));
        check(header[1] < 65536, "bounded frame");
        std::string payload(header[1], '\0'); bytes(payload.data(), header[1]);
        return {header[0], payload};
    }
    void write(unsigned opcode, const std::string& payload, bool fragmented = false) {
        unsigned header[2]{opcode, static_cast<unsigned>(payload.size())};
        std::string frame(reinterpret_cast<const char*>(header), sizeof(header)); frame += payload;
        DWORD count = 0;
        if (fragmented) {
            check(WriteFile(pipe, frame.data(), 3, &count, nullptr) && count == 3, "write partial header");
            std::this_thread::sleep_for(100ms);
            frame.erase(0, 3);
        }
        check(WriteFile(pipe, frame.data(), static_cast<DWORD>(frame.size()), &count, nullptr) && count == frame.size(), "write frame");
    }
    Json handshakeAndActivity() {
        connect();
        const auto handshake = read();
        check(handshake.first == 0 && Json::parse(handshake.second)["client_id"] == "123456789012345678", "handshake ID");
        write(1, R"({"cmd":"DISPATCH","evt":"READY","data":{}})", true);
        const auto activity = read();
        check(activity.first == 1, "activity opcode");
        const auto message = Json::parse(activity.second);
        check(message["cmd"] == "SET_ACTIVITY" && message["args"]["pid"] == GetCurrentProcessId(), "activity command and pid");
        write(1, Json{{"cmd", "SET_ACTIVITY"}, {"nonce", message["nonce"]}, {"evt", nullptr}}.dump());
        return message;
    }
};

int main() {
    try {
        struct ArtworkFixture {
            std::filesystem::path path = std::filesystem::temp_directory_path() /
                ("fishing-artwork-" + std::to_string(GetCurrentProcessId()) + ".json");
            ~ArtworkFixture() { std::error_code error; std::filesystem::remove(path, error); }
            void write(const std::string& value) { std::ofstream out(path); out << value; }
        } fixture;
        check(DiscordPresence::readArtwork(fixture.path).largeImage.empty(), "missing artwork keeps text presence");
        fixture.write(R"({"large_image":"taskbar_fishing","large_text":"Gone fishing"})");
        const auto artwork = DiscordPresence::readArtwork(fixture.path);
        check(artwork.largeImage == "taskbar_fishing" && artwork.largeText == "Gone fishing", "load asset key and hover text");
        fixture.write(R"({"large_image":"https://cdn.discordapp.com/icons/example.png"})");
        check(DiscordPresence::readArtwork(fixture.path).largeImage.starts_with("https://"), "external HTTPS image supported");
        for (const auto* invalid : {"broken", "[]", R"({"large_image":42})", R"({"large_image":"D:/avatar.png"})", R"({"large_image":"http://example.com/a.png"})"}) {
            fixture.write(invalid);
            check(DiscordPresence::readArtwork(fixture.path).largeImage.empty(), "bad artwork falls back safely");
        }
        check(!DiscordPresence::validApplicationId("") && !DiscordPresence::validApplicationId("# placeholder") &&
            !DiscordPresence::validApplicationId("12345678901234567x"), "reject invalid IDs");
        FishingSystem fishing(42); Player player;
        const FishInstance fish{0, 1.0f, 8, false};
        for (int i = 0; i < player.capacity(); ++i) check(player.keep(fish), "fill box");
        check(fishing.restoreCatch(fish), "restore catch");
        fishing.update(3.0f);
        check(DiscordPresence::activityFor(fishing, player, false).details.find("full") != std::string::npos, "full box presence");
        check(DiscordPresence::activityFor(fishing, player, true).details == "Caught Bluegill!", "auto sell does not report blocked box");
        DiscordActivity expected{"Caught a \"fish\"!", "Rod Lv.1 | Fish Box 5/5"};
        Server server;
        auto presence = std::make_unique<DiscordPresence>("123456789012345678", artwork);
        presence->update(expected);
        const auto first = server.handshakeAndActivity();
        check(first["args"]["activity"]["details"] == expected.details, "JSON escaping");
        check(first["args"]["activity"]["state"] == expected.state, "game stats");
        check(first["args"]["activity"]["assets"]["large_image"] == "taskbar_fishing", "avatar is sent in the IPC activity");
        check(first["args"]["activity"]["assets"]["large_text"] == "Gone fishing", "avatar hover text is sent");
        server.write(3, "ping payload", true);
        const auto pong = server.read();
        check(pong.first == 4 && pong.second == "ping payload", "ping pong");
        // A changed snapshot must not be sent again before the 15-second throttle.
        presence->update({"Waiting for a bite", expected.state});
        std::this_thread::sleep_for(200ms);
        DWORD available = 0;
        check(PeekNamedPipe(server.pipe, nullptr, 0, nullptr, &available, nullptr) && available == 0, "throttle updates");
        server.write(2, "{}");
        DisconnectNamedPipe(server.pipe);
        const auto second = server.handshakeAndActivity();
        check(second["args"]["activity"]["details"] == "Waiting for a bite", "latest snapshot on reconnect");
        check(first["args"]["activity"]["timestamps"] == second["args"]["activity"]["timestamps"], "stable session timer");
        check(first["args"]["activity"]["assets"] == second["args"]["activity"]["assets"], "avatar survives reconnect");
        const auto before = std::chrono::steady_clock::now();
        presence.reset();
        check(std::chrono::steady_clock::now() - before < 1s, "bounded shutdown");
        const auto clear = server.read();
        check(Json::parse(clear.second)["args"]["activity"].is_null(), "clear on exit");
        DisconnectNamedPipe(server.pipe);
        { DiscordPresence disabled(""); disabled.update(expected); }
        // No desktop client responding: destructor must not wait for the handshake timeout.
        {
            DiscordPresence unavailable("123456789012345678");
            std::this_thread::sleep_for(100ms);
        }
        std::cout << "Discord Presence tests passed\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
