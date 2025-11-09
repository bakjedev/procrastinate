#pragma once

#include <span>
#include <vulkan/vulkan.hpp>

class VulkanShader {
 public:
  VulkanShader(vk::Device device, std::span<const uint32_t> code);
  VulkanShader(const VulkanShader &) = delete;
  VulkanShader(VulkanShader &&) = delete;
  VulkanShader &operator=(const VulkanShader &) = delete;
  VulkanShader &operator=(VulkanShader &&) = delete;
  ~VulkanShader();

  [[nodiscard]] vk::ShaderModule get() const { return m_shaderModule; }

 private:
  vk::ShaderModule m_shaderModule;

  vk::Device m_device;
};