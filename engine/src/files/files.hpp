#pragma once

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <optional>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

namespace fs = std::filesystem;

namespace files
{
  inline constexpr int kAssetDirSearchLevels = 5;

  inline const fs::path& GetWorkingDirectory()
  {
    static const fs::path dir = []
    {
#if defined(_WIN32)
      char buf[MAX_PATH];
      GetModuleFileNameA(nullptr, buf, MAX_PATH);
      return fs::path(buf).parent_path();
#elif defined(__linux__)
      return fs::read_symlink("/proc/self/exe").parent_path();
#else
      throw std::runtime_error("Unsupported platform");
#endif
    }();
    return dir;
  }

  inline fs::path GetAssetsPathRoot()
  {
    const auto& exe_dir = GetWorkingDirectory();

    auto current = exe_dir;
    for (int i = 0; i < kAssetDirSearchLevels; ++i)
    {
      if (fs::exists(current / "engine/assets") || fs::exists(current / "runtime/assets"))
      {
        return current;
      }
      current = current.parent_path();
    }

    throw std::runtime_error("Resource root not found " + exe_dir.string());
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
