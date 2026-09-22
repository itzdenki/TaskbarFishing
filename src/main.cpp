#include "Game.h"
#include "ui/BackgroundDemo.h"
#include "platform/Diagnostics.h"
#include <exception>
#include <iostream>
#include <string_view>

int main(int argc, char** argv) {
    bool unattended = false;
    for (int i = 1; i < argc; ++i) if (std::string_view(argv[i]) == "--smoke-test") unattended = true;
    diagnostics::initialize(unattended);
    try {
        bool smoke = false;
        bool discordEnabled = true;
        bool backgroundDemo = false;
        bool showStats = false;
        std::filesystem::path savePath;
        for (int i = 1; i < argc; ++i) {
            const std::string_view option(argv[i]);
            if (option == "--smoke-test") smoke = true;
            else if (option == "--no-discord") discordEnabled = false;
            else if (option == "--background-demo") backgroundDemo = true;
            else if (option == "--stats") showStats = true;
            else if (option == "--save-path" && i + 1 < argc) savePath = argv[++i];
            else { std::cerr << "Usage: TaskbarFishing [--smoke-test] [--save-path FILE] [--no-discord] [--background-demo] [--stats]\n"; return 2; }
        }
        if (backgroundDemo) return runBackgroundDemo(smoke);
        Game game(smoke, savePath, discordEnabled);
        game.run(showStats);
        return 0;
    } catch (const std::exception& error) {
        diagnostics::reportError(error.what());
        return 1;
    } catch (...) {
        diagnostics::reportError("Unknown startup error");
        return 1;
    }
}
