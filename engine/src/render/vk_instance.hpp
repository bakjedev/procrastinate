#pragma once
#include <vulkan/vulkan.h>

class VulkanInstance {
 public:
  VulkanInstance();
  VulkanInstance(const VulkanInstance &) = delete;
  VulkanInstance(VulkanInstance &&) = delete;
  VulkanInstance &operator=(const VulkanInstance &) = delete;
  VulkanInstance &operator=(VulkanInstance &&) = delete;
  ~VulkanInstance();
  [[nodiscard]] VkInstance get() const { return m_instance; }

 private:
  VkInstance m_instance{};
};