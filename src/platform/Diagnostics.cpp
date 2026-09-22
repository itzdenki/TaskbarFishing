#include "Diagnostics.h"
#include <raylib.h>
#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iterator>
#include <cwchar>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
// Avoid collisions with raylib's Windows-like API names.
#define CloseWindow Win32CloseWindow
#define ShowCursor Win32ShowCursor
#define DrawText Win32DrawText
#define DrawTextEx Win32DrawTextEx
#define Rectangle Win32Rectangle
#include <windows.h>
#undef CloseWindow
#undef ShowCursor
#undef DrawText
#undef DrawTextEx
#undef Rectangle
#endif

namespace {
bool silent = false;
#ifdef _WIN32
HANDLE logFile = INVALID_HANDLE_VALUE;
wchar_t logPath[32768]{};
#else
FILE* logFile = nullptr;
#endif

void writeLine(const char* message) noexcept {
    std::fprintf(stderr, "%s\n", message);
    std::fflush(stderr);
#ifdef _WIN32
    if (logFile != INVALID_HANDLE_VALUE) {
        DWORD written = 0;
        WriteFile(logFile, message, static_cast<DWORD>(std::char_traits<char>::length(message)), &written, nullptr);
        WriteFile(logFile, "\r\n", 2, &written, nullptr);
        FlushFileBuffers(logFile);
    }
#else
    if (logFile) { std::fprintf(logFile, "%s\n", message); std::fflush(logFile); }
#endif
}

void trace(int level, const char* format, va_list args) {
    char message[4096]{};
    std::vsnprintf(message, sizeof(message), format, args);
    writeLine(message);
    // raylib delegates fatal handling to its callback. Never return from fatal.
    if (level == LOG_FATAL) { diagnostics::reportError(message); std::_Exit(1); }
}

#ifdef _WIN32
LONG WINAPI onCrash(EXCEPTION_POINTERS* exception) {
    // Use a fixed buffer and native file writes; do not allocate or unwind after
    // a native fault. Keep normal Windows Error Reporting available as well.
    char message[1024]{};
    char modulePath[MAX_PATH]{};
    MEMORY_BASIC_INFORMATION memory{};
    const auto* record = exception->ExceptionRecord;
    VirtualQuery(record->ExceptionAddress, &memory, sizeof(memory));
    GetModuleFileNameA(static_cast<HMODULE>(memory.AllocationBase), modulePath, MAX_PATH);
    const int length = std::snprintf(message, sizeof(message),
        "NATIVE CRASH: exception=0x%08lX address=%p module=%s base=%p\r\n",
        record->ExceptionCode, record->ExceptionAddress, modulePath, memory.AllocationBase);
    if (logFile != INVALID_HANDLE_VALUE && length > 0) {
        DWORD written = 0;
        WriteFile(logFile, message, static_cast<DWORD>(length < 1024 ? length : 1023), &written, nullptr);
        FlushFileBuffers(logFile);
    }
    return EXCEPTION_CONTINUE_SEARCH;
}
#endif
}

void diagnostics::initialize(bool unattended) noexcept {
    silent = unattended;
    try {
#ifdef _WIN32
        const wchar_t* local = _wgetenv(L"LOCALAPPDATA");
        auto directory = local ? std::filesystem::path(local) : std::filesystem::temp_directory_path();
        directory /= "TaskbarFishing";
        std::filesystem::create_directories(directory);
        const auto path = directory / "startup.log";
        if (path.native().size() < std::size(logPath)) {
            std::copy(path.native().begin(), path.native().end(), logPath);
            logFile = CreateFileW(logPath, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                  nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        }
#else
        logFile = std::fopen("TaskbarFishing-startup.log", "w");
#endif
    } catch (...) { /* Logging failure must not prevent the game from starting. */ }
#ifdef _WIN32
    SetUnhandledExceptionFilter(onCrash);
#endif
    SetTraceLogCallback(trace);
    std::set_terminate([] {
        diagnostics::reportError("Unexpected C++ termination (possibly in a worker thread).");
        std::_Exit(1);
    });
    writeLine("Taskbar Fishing startup - compatibility build " __DATE__ " " __TIME__);
}

void diagnostics::reportError(const char* message) noexcept {
    writeLine(message);
#ifdef _WIN32
    if (!silent) {
        wchar_t detail[2048]{};
        MultiByteToWideChar(CP_UTF8, 0, message, -1, detail, static_cast<int>(std::size(detail)));
        wchar_t dialog[32768]{};
        const wchar_t* location = logFile != INVALID_HANDLE_VALUE ? logPath : L"(log file unavailable)";
        std::swprintf(dialog, std::size(dialog),
            L"Taskbar Fishing could not run.\n\n%ls\n\nDiagnostic log:\n%ls\n\nIf the log mentions GLFW/OpenGL, check the display driver.",
            detail, location);
        MessageBoxW(nullptr, dialog, L"Taskbar Fishing - startup error", MB_OK | MB_ICONERROR);
    }
#endif
}
