#pragma once

#include <string>
#include <vector>

#include "files/files.hpp"

struct ShaderResource
{
  std::vector<uint32_t> code;

  explicit ShaderResource(std::vector<uint32_t> data) : code(std::move(data)) {}
};

struct ShaderResourceLoader
{
  ShaderResource operator()(const std::string& path) const
  {
    auto data = files::ReadBinaryFile(path);
    if (!data)
    {
      throw std::runtime_error("Failed to load shader: " + path);
    }
    return ShaderResource{std::move(*data)};
  }
};
