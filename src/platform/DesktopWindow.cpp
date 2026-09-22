#include "platform/DesktopWindow.h"
#include <algorithm>
#include <string>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shellapi.h>
#endif

namespace desktop {
#ifdef _WIN32
namespace {
constexpr wchar_t kRunKey[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
constexpr wchar_t kRunValue[] = L"TaskbarFishing";

std::wstring launchCommand() {
    wchar_t path[MAX_PATH]{};
    const DWORD length = GetModuleFileNameW(nullptr, path, MAX_PATH);
    if (length == 0 || length >= MAX_PATH) return {};
    return L"\"" + std::wstring(path, length) + L"\"";
}
std::wstring registeredCommand(HKEY key) {
    wchar_t existing[1024]{};
    DWORD size = sizeof(existing), type = 0;
    if (RegQueryValueExW(key, kRunValue, nullptr, &type, reinterpret_cast<LPBYTE>(existing), &size) != ERROR_SUCCESS || type != REG_SZ) return {};
    existing[std::min<DWORD>(size / sizeof(wchar_t), 1023)] = 0;
    return existing;
}
}
#endif

void keepInWorkArea(void* handle) {
#ifdef _WIN32
    const auto window = static_cast<HWND>(handle);
    MONITORINFO info{};
    info.cbSize = sizeof(info);
    RECT bounds{};
    if (GetWindowRect(window, &bounds) && GetMonitorInfoW(MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST), &info)) {
        const auto& area = info.rcWork;
        const int maxX = area.right - (bounds.right - bounds.left);
        const int maxY = area.bottom - (bounds.bottom - bounds.top);
        const int x = bounds.left < area.left ? area.left : bounds.left > maxX ? maxX : bounds.left;
        const int y = bounds.top < area.top ? area.top : bounds.top > maxY ? maxY : bounds.top;
        SetWindowPos(window, nullptr, x < area.left ? area.left : x, y < area.top ? area.top : y,
                     0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    }
#else
    (void)handle;
#endif
}

void dockAboveTaskbar(void* handle, DockOptions options) {
#ifdef _WIN32
    const auto window = static_cast<HWND>(handle);
    MONITORINFO info{};
    info.cbSize = sizeof(info);
    const HMONITOR monitor = options.primaryMonitor ? MonitorFromPoint(POINT{0, 0}, MONITOR_DEFAULTTOPRIMARY)
                                                    : MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST);
    RECT bounds{};
    if (GetMonitorInfoW(monitor, &info) && GetWindowRect(window, &bounds)) {
        const auto& area = info.rcWork;
        const int width = bounds.right - bounds.left, height = bounds.bottom - bounds.top;
        const int gap = options.gap < 0 ? 0 : options.gap;
        const int space = area.right - area.left - width;
        const int x = area.left + (space > 0 ? options.alignment == 0 ? 0 : options.alignment == 2 ? space : space / 2 : 0);
        const int y = (area.bottom - height - gap > area.top) ? area.bottom - height - gap : area.top;
        SetWindowPos(window, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    }
#else
    (void)handle; (void)options;
#endif
}
bool cursorPosition(int& x, int& y) {
#ifdef _WIN32
    POINT point{};
    if (!GetCursorPos(&point)) return false;
    x = point.x; y = point.y;
    return true;
#else
    (void)x; (void)y;
    return false;
#endif
}
bool leftButtonDown() {
#ifdef _WIN32
    return (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
#else
    return false;
#endif
}
bool cursorOverWindow(void* handle) {
#ifdef _WIN32
    POINT point{};
    RECT bounds{};
    if (!GetCursorPos(&point) || !GetWindowRect(static_cast<HWND>(handle), &bounds)) return false;
    return PtInRect(&bounds, point) != 0;
#else
    (void)handle;
    return false;
#endif
}
bool setStartWithWindows(bool enabled) {
#ifdef _WIN32
    HKEY key = nullptr;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, kRunKey, 0, nullptr, 0, KEY_SET_VALUE | KEY_QUERY_VALUE, nullptr, &key, nullptr) != ERROR_SUCCESS) return false;
    bool ok = false;
    if (enabled) {
        const auto command = launchCommand();
        // Rewrite only when the executable moved, so startup does not touch the registry needlessly.
        if (!command.empty()) {
            ok = registeredCommand(key) == command ||
                 RegSetValueExW(key, kRunValue, 0, REG_SZ, reinterpret_cast<const BYTE*>(command.c_str()),
                                static_cast<DWORD>((command.size() + 1) * sizeof(wchar_t))) == ERROR_SUCCESS;
        }
    } else {
        const auto result = RegDeleteValueW(key, kRunValue);
        ok = result == ERROR_SUCCESS || result == ERROR_FILE_NOT_FOUND;
    }
    RegCloseKey(key);
    return ok;
#else
    (void)enabled;
    return false;
#endif
}
bool startsWithWindows() {
#ifdef _WIN32
    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, kRunKey, 0, KEY_QUERY_VALUE, &key) != ERROR_SUCCESS) return false;
    const bool registered = !registeredCommand(key).empty();
    RegCloseKey(key);
    return registered;
#else
    return false;
#endif
}
bool openFolder(const std::filesystem::path& folder) {
#ifdef _WIN32
    const auto result = ShellExecuteW(nullptr, L"open", folder.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    return reinterpret_cast<INT_PTR>(result) > 32;
#else
    (void)folder;
    return false;
#endif
}
}
