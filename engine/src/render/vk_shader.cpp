#include "vk_shader.hpp"

VulkanShader::VulkanShader(vk::Device device, std::span<const uint32_t> code) : device_(device)
{
  const vk::ShaderModuleCreateInfo createInfo{.codeSize = code.size_bytes(), .pCode = code.data()};

  shader_module_ = device_.createShaderModule(createInfo);
}

VulkanShader::~VulkanShader()
{
  if (shader_module_)
  {
    device_.destroyShaderModule(shader_module_);
  }
}
