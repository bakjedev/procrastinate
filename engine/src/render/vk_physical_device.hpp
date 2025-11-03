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

  // features
  [[nodiscard]] const VkPhysicalDeviceFeatures& features() const {
    return m_features;
  }
  [[nodiscard]] const VkPhysicalDeviceVulkan12Features& features12() const {
    return m_features12;
  }
  [[nodiscard]] const VkPhysicalDeviceVulkan13Features& features13() const {
    return m_features13;
  }

  [[nodiscard]] std::string name() const { return {m_properties.deviceName}; }

  [[nodiscard]] const QueueFamilyIndices& queueFamilies() const {
    return m_queueFamilyIndices;
  }

 private:
  VkPhysicalDevice m_physicalDevice{};
  VkPhysicalDeviceProperties m_properties{};
  VkPhysicalDeviceFeatures m_features{};
  VkPhysicalDeviceVulkan12Features m_features12{};
  VkPhysicalDeviceVulkan13Features m_features13{};

  QueueFamilyIndices m_queueFamilyIndices{};

  static bool isDeviceSuitable(const VkPhysicalDeviceProperties& properties,
                               const VkPhysicalDeviceFeatures& features);

  bool findQueueFamilies(VkSurfaceKHR surface);
};