#include "EmbeddedAssets.h"
#include <fstream>
#include <iterator>
#include <array>
#include <stdexcept>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace assets {
std::filesystem::path applicationDirectory(const char* fallback) {
#ifdef _WIN32
    (void)fallback;
    std::array<wchar_t, 32768> path{};
    const auto length = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
    if (length == 0 || length >= path.size()) throw std::runtime_error("Could not read executable path");
    return std::filesystem::path(path.data()).parent_path();
#else
    return std::filesystem::path(fallback);
#endif
}

std::span<const unsigned char> embedded(std::string_view name) {
#ifdef _WIN32
    struct Entry { std::string_view name; int id; };
    static constexpr Entry entries[]{
#include "EmbeddedAssetIndex.inc"
    };
    for (const auto& entry : entries) {
        if (entry.name != name) continue;
        const auto module = GetModuleHandleW(nullptr);
        const auto resource = FindResourceW(module, MAKEINTRESOURCEW(entry.id), MAKEINTRESOURCEW(10));
        if (!resource) return {};
        const auto size = SizeofResource(module, resource);
        const auto loaded = LoadResource(module, resource);
        const auto* data = loaded ? static_cast<const unsigned char*>(LockResource(loaded)) : nullptr;
        return data ? std::span<const unsigned char>(data, size) : std::span<const unsigned char>{};
    }
#else
    (void)name;
#endif
    return {};
}

std::string readText(const std::filesystem::path& file) {
    std::ifstream input(file, std::ios::binary);
    if (input) return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
    const auto data = embedded(file.filename().string());
    return data.empty() ? std::string{} : std::string(reinterpret_cast<const char*>(data.data()), data.size());
}
}
