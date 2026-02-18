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
  [[nodiscard]] vk::Instance get() const { return m_instance; }
  [[nodiscard]] vk::detail::DispatchLoaderDynamic &getDynamicLoader() { return m_dynamicLoader; }

private:
  vk::Instance m_instance;

  vk::detail::DispatchLoaderDynamic m_dynamicLoader;
#ifndef NDEBUG
  vk::DebugUtilsMessengerEXT m_debugMessenger;
#endif
};
