#pragma once

#include <vulkan/vulkan.h>

#include <span>

class VulkanShader {
 public:
  VulkanShader(VkDevice device, std::span<const uint32_t> code);
  VulkanShader(const VulkanShader &) = delete;
  VulkanShader(VulkanShader &&) = delete;
  VulkanShader &operator=(const VulkanShader &) = delete;
  VulkanShader &operator=(VulkanShader &&) = delete;
  ~VulkanShader();

 private:
  VkShaderModule m_shaderModule = VK_NULL_HANDLE;

  VkDevice m_device = VK_NULL_HANDLE;
};