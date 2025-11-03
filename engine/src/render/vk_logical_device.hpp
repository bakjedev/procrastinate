#pragma once
#include <vulkan/vulkan.h>

struct QueueFamilyIndices;

class VulkanLogicalDevice {
 public:
  explicit VulkanLogicalDevice(VkPhysicalDevice physicalDevice,
                               const VkPhysicalDeviceFeatures& features,
                               const QueueFamilyIndices& queueFamilyIndices);
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