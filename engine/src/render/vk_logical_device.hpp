#pragma once
#include <vulkan/vulkan.h>

struct QueueFamilyIndices;

class VulkanLogicalDevice {
 public:
  VulkanLogicalDevice(VkPhysicalDevice physicalDevice,
                      const VkPhysicalDeviceFeatures& features,
                      const QueueFamilyIndices& queueFamilyIndices,
                      const VkPhysicalDeviceVulkan12Features& features12,
                      const VkPhysicalDeviceVulkan13Features& features13);
  VulkanLogicalDevice(const VulkanLogicalDevice&) = delete;
  VulkanLogicalDevice(VulkanLogicalDevice&&) = delete;
  VulkanLogicalDevice& operator=(const VulkanLogicalDevice&) = delete;
  VulkanLogicalDevice& operator=(VulkanLogicalDevice&&) = delete;
  ~VulkanLogicalDevice();

  [[nodiscard]] VkDevice get() const { return m_logicalDevice; }
  [[nodiscard]] VkQueue graphicsQueue() const { return m_graphicsQueue; }
  [[nodiscard]] VkQueue computeQueue() const { return m_computeQueue; }
  [[nodiscard]] VkQueue transferQueue() const { return m_transferQueue; }
  [[nodiscard]] VkQueue presentQueue() const { return m_presentQueue; }

  void waitIdle();

 private:
  VkDevice m_logicalDevice{};

  VkQueue m_graphicsQueue{};
  VkQueue m_presentQueue{};
  VkQueue m_transferQueue{};
  VkQueue m_computeQueue{};
};