#pragma once
#include <filesystem>

namespace desktop {
struct DockOptions {
    int alignment = 1;          // 0 left, 1 center, 2 right
    int gap = 8;                // pixels above the taskbar
    bool primaryMonitor = false; // true recovers a window lost on a disconnected display
};
void dockAboveTaskbar(void* handle, DockOptions options = {});
void keepInWorkArea(void* handle);
bool cursorPosition(int& x, int& y);
bool leftButtonDown();
bool cursorOverWindow(void* handle);
// HKCU Run entry pointing at this executable. Returns false when the registry refused the change.
bool setStartWithWindows(bool enabled);
bool startsWithWindows();
bool openFolder(const std::filesystem::path& folder);
}
