#pragma once

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <optional>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif
namespace files
{

  inline constexpr int kAssetDirSearchLevels = 5;

  inline const std::filesystem::path& GetWorkingDirectory()
  {
    static const auto dir = std::filesystem::current_path();
    return dir;
  }

  inline std::filesystem::path GetAssetsPathRoot()
  {
    const auto exe_dir = GetWorkingDirectory().parent_path();

    auto current = exe_dir;
    for (int i = 0; i < kAssetDirSearchLevels; ++i)
    {
      if (std::filesystem::exists(current / "engine/assets") || std::filesystem::exists(current / "runtime/assets"))
      {
        return current;
      }
      current = current.parent_path();
    }

    throw std::runtime_error("Resource root not found");
  }

  inline std::optional<std::vector<uint32_t>> ReadBinaryFile(const std::string& path)
  {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open())
    {
      return std::nullopt;
    }

    const auto size = file.tellg();
    file.seekg(0, std::ios::beg);

    if (size % sizeof(uint32_t) != 0)
    {
      return std::nullopt;
    }

    std::vector<uint32_t> buffer(size / sizeof(uint32_t));
    file.read(reinterpret_cast<char*>(buffer.data()), size);

    if (!file)
    {
      return std::nullopt;
    }

    return buffer;
  }
} // namespace files
