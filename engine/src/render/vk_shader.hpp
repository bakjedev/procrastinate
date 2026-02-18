#pragma once

#include <span>
#include <vulkan/vulkan.hpp>

class VulkanShader
{
public:
  VulkanShader(vk::Device device, std::span<const uint32_t> code);
  VulkanShader(const VulkanShader &) = delete;
  VulkanShader(VulkanShader &&) = delete;
  VulkanShader &operator=(const VulkanShader &) = delete;
  VulkanShader &operator=(VulkanShader &&) = delete;
  ~VulkanShader();

  [[nodiscard]] vk::ShaderModule get() const { return shader_module_; }

private:
  vk::ShaderModule shader_module_;

  vk::Device device_;
};
