#pragma once
#include <vulkan/vulkan.hpp>

class VulkanInstance
{
public:
  VulkanInstance();
  VulkanInstance(const VulkanInstance &) = delete;
  VulkanInstance(VulkanInstance &&) = delete;
  VulkanInstance &operator=(const VulkanInstance &) = delete;
  VulkanInstance &operator=(VulkanInstance &&) = delete;
  ~VulkanInstance();
  [[nodiscard]] vk::Instance get() const { return instance_; }
  [[nodiscard]] vk::detail::DispatchLoaderDynamic &getDynamicLoader() { return dynamic_loader_; }

private:
  vk::Instance instance_;

  vk::detail::DispatchLoaderDynamic dynamic_loader_;
#ifndef NDEBUG
  vk::DebugUtilsMessengerEXT debug_messenger_;
#endif
};
