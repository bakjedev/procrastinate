#pragma once

#include <string>
#include <vector>

#include "files/files.hpp"

struct ShaderResource
{
  std::vector<uint32_t> code;

  explicit ShaderResource(const std::string& path) { code = files::ReadBinaryFile(path).value(); }
};

struct ShaderResourceLoader
{
  ShaderResource operator()(const std::string& path) const { return ShaderResource{path}; }
};
