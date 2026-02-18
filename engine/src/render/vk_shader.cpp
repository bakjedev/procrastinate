#include "vk_shader.hpp"

VulkanShader::VulkanShader(const vk::Device device, std::span<const uint32_t> code) : device_(device)
{
  const vk::ShaderModuleCreateInfo create_info{.codeSize = code.size_bytes(), .pCode = code.data()};

  shader_module_ = device_.createShaderModule(create_info);
}

VulkanShader::~VulkanShader()
{
  if (shader_module_)
  {
    device_.destroyShaderModule(shader_module_);
  }
}
