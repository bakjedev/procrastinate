#pragma once
#include <vulkan/vulkan.h>

#include <optional>
#include <string>

struct QueueFamilyIndices {
  std::optional<uint32_t> graphics;
  std::optional<uint32_t> compute;
  std::optional<uint32_t> transfer;
  std::optional<uint32_t> present;

  [[nodiscard]] bool isComplete() const {
    return graphics.has_value() && present.has_value() && compute.has_value() &&
           transfer.has_value();
  }
};

class VulkanPhysicalDevice {
 public:
  explicit VulkanPhysicalDevice(VkInstance instance, VkSurfaceKHR surface);
  ~VulkanPhysicalDevice();

  [[nodiscard]] VkPhysicalDevice get() const { return m_physicalDevice; }
  [[nodiscard]] const VkPhysicalDeviceFeatures& features() const {
    return m_features;
  }
  [[nodiscard]] std::string name() const { return {m_properties.deviceName}; }

  [[nodiscard]] const QueueFamilyIndices& queueFamilies() const {
    return m_queueFamilyIndices;
  }

 private:
  VkPhysicalDevice m_physicalDevice{};
  VkPhysicalDeviceProperties m_properties{};
  VkPhysicalDeviceFeatures m_features{};

  QueueFamilyIndices m_queueFamilyIndices{};

  static bool isDeviceSuitable(const VkPhysicalDeviceProperties& properties,
                               const VkPhysicalDeviceFeatures& features);

  bool findQueueFamilies(VkSurfaceKHR surface);
};