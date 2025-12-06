#pragma once

#include <cstdint>
#include <fstream>
#include <optional>
#include <vector>
#include <filesystem>

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
    
    auto current = exe_dir;
    for (int i = 0; i < 5; ++i) {
        if (std::filesystem::exists(current / "engine/assets") ||
            std::filesystem::exists(current / "runtime/assets")) {
            return current;
        }
        current = current.parent_path();
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
