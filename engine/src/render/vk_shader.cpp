#include "vk_shader.hpp"

VulkanShader::VulkanShader(vk::Device device, std::span<const uint32_t> code)
    : m_device(device) {
  vk::ShaderModuleCreateInfo createInfo{.codeSize = code.size_bytes(),
                                        .pCode = code.data()};

  m_shaderModule = m_device.createShaderModule(createInfo);
}

VulkanShader::~VulkanShader() {
  if (m_shaderModule) {
    m_device.destroyShaderModule(m_shaderModule);
  }
}