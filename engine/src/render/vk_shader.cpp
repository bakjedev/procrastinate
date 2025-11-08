#include "vk_shader.hpp"

#include "util/vk_check.hpp"

VulkanShader::VulkanShader(VkDevice device, std::span<const uint32_t> code)
    : m_device(device) {
  VkShaderModuleCreateInfo createInfo{
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0U,
      .codeSize = code.size_bytes(),
      .pCode = code.data()};

  VK_CHECK(vkCreateShaderModule(device, &createInfo, nullptr, &m_shaderModule));
}

VulkanShader::~VulkanShader() {
  if (m_shaderModule != VK_NULL_HANDLE) {
    vkDestroyShaderModule(m_device, m_shaderModule, nullptr);
  }
}