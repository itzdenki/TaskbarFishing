#pragma once
#include <filesystem>
#include <span>
#include <string>
#include <string_view>

namespace assets {
// Views refer to executable resources, valid for the lifetime of the process.
std::span<const unsigned char> embedded(std::string_view name);
// An explicit configuration file takes precedence, including an intentionally empty file.
std::string readText(const std::filesystem::path& file);
// Windows uses the wide-character module path. Other platforms use raylib's path.
std::filesystem::path applicationDirectory(const char* fallback);
}
