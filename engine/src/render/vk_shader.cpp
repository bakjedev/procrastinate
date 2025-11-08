#include "vk_shader.hpp"

VulkanShader::VulkanShader(VkDevice device, std::span<const uint32_t> code)
    : m_device(device) {
  VkShaderModuleCreateInfo createInfo{
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0U,
      .codeSize = code.size(),
      .pCode = code.data()};

  vkCreateShaderModule(device, &createInfo, nullptr, &m_shaderModule);
}

VulkanShader::~VulkanShader() {
  if (m_shaderModule != VK_NULL_HANDLE) {
    vkDestroyShaderModule(m_device, m_shaderModule, nullptr);
  }
}