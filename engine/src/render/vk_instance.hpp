#pragma once
#include <vulkan/vulkan.hpp>

class VulkanInstance {
 public:
  VulkanInstance();
  VulkanInstance(const VulkanInstance &) = delete;
  VulkanInstance(VulkanInstance &&) = delete;
  VulkanInstance &operator=(const VulkanInstance &) = delete;
  VulkanInstance &operator=(VulkanInstance &&) = delete;
  ~VulkanInstance();
  [[nodiscard]] vk::Instance get() const { return m_instance; }

 private:
  vk::Instance m_instance;
};