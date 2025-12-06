#pragma once

#include <cstdint>
#include <fstream>
#include <optional>
#include <vector>
#include <filesystem>
#include "util/print.hpp"

namespace Files {


inline std::filesystem::path getExecutablePath() {
#ifdef _WIN32
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    return std::filesystem::path(buffer);
#elif __linux__
    return std::filesystem::canonical("/proc/self/exe");
#else
    #error "Unsupported platform"
#endif
}

inline std::filesystem::path getResourceRoot() {
    auto exe_dir = getExecutablePath().parent_path();
    
    // Search upward for project root marker
    auto current = exe_dir;
    Util::println("current exe dir: {}", current.string());
    for (int i = 0; i < 5; ++i) {  // Limit search depth
        if (std::filesystem::exists(current / "engine/assets") ||
            std::filesystem::exists(current / "runtime/assets")) {
            Util::println("They exist in {}", current.string());
            return current;
        }
        current = current.parent_path();
        Util::println("They didn't exist in {}", current.string());
    }
    
    throw std::runtime_error("Resource root not found");
}

inline std::optional<std::vector<uint32_t>> readBinaryFile(
    const std::string& path) {
  std::ifstream file(path, std::ios::binary | std::ios::ate);
  if (!file.is_open()) {
    return std::nullopt;
  }

  const auto size = file.tellg();
  file.seekg(0, std::ios::beg);

  if (size % sizeof(uint32_t) != 0) {
    return std::nullopt;
  }

  std::vector<uint32_t> buffer(size / sizeof(uint32_t));
  file.read(reinterpret_cast<char*>(buffer.data()), size);

  if (!file) {
    return std::nullopt;
  }

  return buffer;
}
}  // namespace Files
