#pragma once

#include <cstddef>
#include <fstream>
#include <optional>
#include <vector>

namespace Files {
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