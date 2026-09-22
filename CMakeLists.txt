cmake_minimum_required(VERSION 3.24)
project(TaskbarFishing VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "Compatibility for raylib 5.5")

# This 2D game does not use shaders or render targets. A legacy context also
# works on older display drivers that cannot create a 3.3 core context.
if(WIN32)
    set(OPENGL_VERSION "1.1" CACHE STRING "OpenGL backend (1.1 for Windows compatibility)")
endif()
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

include(FetchContent)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(raylib
    URL https://github.com/raysan5/raylib/archive/refs/tags/5.5.tar.gz
    URL_HASH SHA256=aea98ecf5bc5c5e0b789a76de0083a21a70457050ea4cc2aec7566935f5e258e
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_MakeAvailable(raylib)
include(cmake/PatchRaylibStartup.cmake)

set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
set(JSON_Install OFF CACHE BOOL "" FORCE)
FetchContent_Declare(nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
    URL_HASH SHA256=42f6e95cad6ec532fd372391373363b62a14af6d771056dbfc86160e6dfff7aa
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_MakeAvailable(nlohmann_json)
find_package(Threads REQUIRED)

add_library(fishing_core src/fishing/FishingSystem.cpp src/fish/FishDatabase.cpp src/player/Player.cpp src/save/SaveSystem.cpp src/platform/LocalTime.cpp src/platform/EmbeddedAssets.cpp)
target_include_directories(fishing_core PUBLIC src)
target_include_directories(fishing_core PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
add_library(fishing_discord src/platform/DiscordPresence.cpp)
target_include_directories(fishing_discord PUBLIC src)
target_link_libraries(fishing_discord PUBLIC fishing_core PRIVATE nlohmann_json::nlohmann_json Threads::Threads)
add_library(fishing_art src/ui/AssetCatalog.cpp)
target_link_libraries(fishing_art PUBLIC fishing_core raylib PRIVATE nlohmann_json::nlohmann_json)
add_executable(TaskbarFishing src/main.cpp src/Game.cpp src/platform/Diagnostics.cpp src/platform/DesktopWindow.cpp src/ui/UI.cpp src/ui/SettingsUI.cpp src/ui/WelcomeUI.cpp src/ui/DashboardUI.cpp src/ui/StatsUI.cpp src/ui/BackgroundDemo.cpp src/ui/HourlyBackground.cpp src/ui/FishingSprites.cpp)
target_include_directories(TaskbarFishing PRIVATE src)
target_link_libraries(TaskbarFishing PRIVATE raylib fishing_core fishing_discord fishing_art)

# Preserve artwork configuration customized beside a local executable when rebuilding.
if(NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/discord_presence.json")
    configure_file(discord_presence.json discord_presence.json COPYONLY)
endif()
add_custom_command(TARGET TaskbarFishing POST_BUILD
    COMMAND ${CMAKE_COMMAND}
        "-DSOURCE=${CMAKE_CURRENT_BINARY_DIR}/discord_presence.json"
        "-DDESTINATION=$<TARGET_FILE_DIR:TaskbarFishing>"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CopyDiscordConfig.cmake"
    VERBATIM)

# Keep artwork beside the executable, independent of the launch working directory.
# copy_if_different also picks up sprite-only edits without requiring a relink.
set(FISHING_SPRITES
    "${CMAKE_CURRENT_SOURCE_DIR}/sprite/bautroi.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/sprite/matnuoc.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/sprite/caugo.png")
add_custom_target(FishingSprites ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:TaskbarFishing>/assets"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/assets/assets.json" "$<TARGET_FILE_DIR:TaskbarFishing>/assets/assets.json"
    VERBATIM)
set(FISHING_RUNTIME_ASSETS "assets/assets.json" "assets/fish/fish.json" "assets/special/special.json")
foreach(GROUP IN ITEMS background player effects equipment ui fish special)
    file(GLOB_RECURSE GROUP_PNGS CONFIGURE_DEPENDS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "assets/${GROUP}/*.png")
    list(APPEND FISHING_RUNTIME_ASSETS ${GROUP_PNGS})
    add_custom_command(TARGET FishingSprites POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/assets/${GROUP}" "$<TARGET_FILE_DIR:TaskbarFishing>/assets/${GROUP}"
        VERBATIM)
    install(DIRECTORY "assets/${GROUP}" DESTINATION assets COMPONENT Game FILES_MATCHING PATTERN "*.png")
endforeach()
install(FILES assets/assets.json DESTINATION assets COMPONENT Game)
install(FILES assets/fish/fish.json DESTINATION assets/fish COMPONENT Game)
install(FILES assets/special/special.json DESTINATION assets/special COMPONENT Game)
add_dependencies(TaskbarFishing FishingSprites)
if(WIN32)
    enable_language(RC)
    configure_file(cmake/AppIcon.rc.in "${CMAKE_CURRENT_BINARY_DIR}/AppIcon.rc" @ONLY)
    target_sources(TaskbarFishing PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/AppIcon.rc")
    set_source_files_properties("${CMAKE_CURRENT_BINARY_DIR}/AppIcon.rc" PROPERTIES
        OBJECT_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/sprite/app-icon.ico")
    # Store all artwork, default configuration and license notices directly in the PE.
    file(GLOB_RECURSE EMBEDDED_FILES CONFIGURE_DEPENDS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "sprite/*")
    list(APPEND EMBEDDED_FILES ${FISHING_RUNTIME_ASSETS})
    list(APPEND EMBEDDED_FILES "discord_presence.json" "README.md" "licenses/RAYLIB.txt" "licenses/GLFW.txt")
    set(EMBEDDED_RC "")
    set(EMBEDDED_INDEX "")
    set(EMBEDDED_DEPENDS "")
    set(ASSET_ID 1000)
    foreach(ASSET_NAME IN LISTS EMBEDDED_FILES)
        set(ASSET_PATH "${CMAKE_CURRENT_SOURCE_DIR}/${ASSET_NAME}")
        if(ASSET_NAME STREQUAL "discord_presence.json")
            set(ASSET_PATH "${CMAKE_CURRENT_BINARY_DIR}/${ASSET_NAME}")
        endif()
        string(APPEND EMBEDDED_RC "${ASSET_ID} RCDATA \"${ASSET_PATH}\"\n")
        string(APPEND EMBEDDED_INDEX "{\"${ASSET_NAME}\", ${ASSET_ID}},\n")
        list(APPEND EMBEDDED_DEPENDS "${ASSET_PATH}")
        math(EXPR ASSET_ID "${ASSET_ID} + 1")
    endforeach()
    string(APPEND EMBEDDED_RC "${ASSET_ID} RCDATA \"${nlohmann_json_SOURCE_DIR}/LICENSE.MIT\"\n")
    string(APPEND EMBEDDED_INDEX "{\"licenses/NLOHMANN_JSON.txt\", ${ASSET_ID}},\n")
    file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/EmbeddedAssets.rc" CONTENT "${EMBEDDED_RC}")
    file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/EmbeddedAssetIndex.inc" CONTENT "${EMBEDDED_INDEX}")
    target_sources(TaskbarFishing PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/EmbeddedAssets.rc")
    set_source_files_properties("${CMAKE_CURRENT_BINARY_DIR}/EmbeddedAssets.rc" PROPERTIES OBJECT_DEPENDS "${EMBEDDED_DEPENDS};${nlohmann_json_SOURCE_DIR}/LICENSE.MIT")
    target_link_libraries(TaskbarFishing PRIVATE user32 advapi32 shell32)
    set_target_properties(TaskbarFishing PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

install(TARGETS TaskbarFishing RUNTIME DESTINATION . COMPONENT Game)
install(TARGETS TaskbarFishing RUNTIME DESTINATION . COMPONENT Standalone)
install(FILES README.md DESTINATION . COMPONENT Game)
install(FILES scripts/BackgroundDemo.cmd DESTINATION . COMPONENT Game)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/discord_presence.json" DESTINATION . COMPONENT Game)
install(FILES sprite/discord/taskbar_fishing.png sprite/discord/README.md DESTINATION sprite/discord COMPONENT Game)
install(FILES sprite/app-icon.png sprite/app-icon.ico DESTINATION sprite COMPONENT Game)
install(FILES "${nlohmann_json_SOURCE_DIR}/LICENSE.MIT" DESTINATION licenses RENAME NLOHMANN_JSON.txt COMPONENT Game)
install(FILES ${FISHING_SPRITES} DESTINATION sprite COMPONENT Game)
install(FILES licenses/RAYLIB.txt licenses/GLFW.txt DESTINATION licenses COMPONENT Game)

include(CTest)
if(BUILD_TESTING)
    add_executable(StatsTests tests/StatsTests.cpp)
    target_link_libraries(StatsTests PRIVATE fishing_core)
    if(MINGW)
        target_link_options(StatsTests PRIVATE -static -static-libgcc -static-libstdc++)
    endif()
    add_test(NAME stats_progression COMMAND StatsTests)
    add_executable(BackgroundClockTests tests/BackgroundTests.cpp)
    target_include_directories(BackgroundClockTests PRIVATE src)
    target_link_libraries(BackgroundClockTests PRIVATE fishing_core)
    if(MINGW)
        target_link_options(BackgroundClockTests PRIVATE -static -static-libgcc -static-libstdc++)
    endif()
    add_test(NAME background_clock COMMAND BackgroundClockTests)
    add_test(NAME background_demo_smoke COMMAND TaskbarFishing --background-demo --smoke-test)
    set_tests_properties(background_demo_smoke PROPERTIES TIMEOUT 30 RUN_SERIAL TRUE)
    if(WIN32)
        add_executable(GraphicsStartupTests tests/GraphicsStartupTests.cpp)
        target_link_libraries(GraphicsStartupTests PRIVATE raylib glfw)
        if(MINGW)
            target_link_options(GraphicsStartupTests PRIVATE -static -static-libgcc -static-libstdc++)
        endif()
        add_test(NAME graphics_startup_failure COMMAND GraphicsStartupTests)
        set_tests_properties(graphics_startup_failure PROPERTIES TIMEOUT 20 RUN_SERIAL TRUE)
        # Compile a separate transport against an isolated pipe; never contact real Discord in tests.
        add_executable(DiscordPresenceTests tests/DiscordTests.cpp src/platform/DiscordPresence.cpp)
        target_link_libraries(DiscordPresenceTests PRIVATE fishing_core nlohmann_json::nlohmann_json Threads::Threads)
        target_compile_definitions(DiscordPresenceTests PRIVATE FISHING_DISCORD_TEST_TRANSPORT)
        if(MINGW)
            target_link_options(DiscordPresenceTests PRIVATE -static -static-libgcc -static-libstdc++)
        endif()
        add_test(NAME discord_presence COMMAND DiscordPresenceTests)
        set_tests_properties(discord_presence PROPERTIES TIMEOUT 25 RUN_SERIAL TRUE)
    endif()
    add_executable(CoreTests tests/CoreTests.cpp)
    target_link_libraries(CoreTests PRIVATE fishing_core)
    if(MINGW)
        target_link_options(CoreTests PRIVATE -static -static-libgcc -static-libstdc++)
    endif()
    add_test(NAME core_gameplay COMMAND CoreTests)
    add_executable(UIIntegrationTests tests/UITests.cpp src/ui/UI.cpp src/ui/SettingsUI.cpp src/ui/WelcomeUI.cpp src/ui/DashboardUI.cpp src/ui/StatsUI.cpp src/ui/HourlyBackground.cpp src/ui/FishingSprites.cpp)
    target_link_libraries(UIIntegrationTests PRIVATE fishing_core raylib fishing_art)
    add_dependencies(UIIntegrationTests FishingSprites)
    if(MINGW)
        target_link_options(UIIntegrationTests PRIVATE -static -static-libgcc -static-libstdc++)
    endif()
    add_test(NAME ui_integration COMMAND UIIntegrationTests)
    add_test(NAME game_smoke COMMAND TaskbarFishing --smoke-test)
    if(WIN32)
        add_test(NAME standalone_smoke COMMAND ${CMAKE_COMMAND}
            "-DGAME_EXE=$<TARGET_FILE:TaskbarFishing>"
            "-DTEST_ROOT=${CMAKE_CURRENT_BINARY_DIR}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/StandaloneSmoke.cmake")
        set_tests_properties(standalone_smoke PROPERTIES TIMEOUT 60 RUN_SERIAL TRUE)
        set_tests_properties(game_smoke background_demo_smoke PROPERTIES
            ENVIRONMENT "LOCALAPPDATA=${CMAKE_CURRENT_BINARY_DIR}/test-profile")
    endif()
    set_tests_properties(ui_integration game_smoke PROPERTIES TIMEOUT 20 RUN_SERIAL TRUE)
endif()
if(MSVC)
    target_compile_options(fishing_discord PRIVATE /W4 /permissive-)
    target_compile_options(TaskbarFishing PRIVATE /W4 /permissive-)
    target_compile_options(fishing_core PRIVATE /W4 /permissive-)
else()
    target_compile_options(fishing_discord PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(TaskbarFishing PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(fishing_core PRIVATE -Wall -Wextra -Wpedantic)
    if(MINGW)
        target_link_options(TaskbarFishing PRIVATE -static -static-libgcc -static-libstdc++)
    endif()
endif()
