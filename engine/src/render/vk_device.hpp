#pragma once
#include <optional>
#include <string>
#include <vulkan/vulkan.hpp>

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

class VulkanDevice {
 public:
  explicit VulkanDevice(vk::Instance instance, vk::SurfaceKHR surface);
  VulkanDevice(const VulkanDevice&) = delete;
  VulkanDevice(VulkanDevice&&) = delete;
  VulkanDevice& operator=(const VulkanDevice&) = delete;
  VulkanDevice& operator=(VulkanDevice&&) = delete;
  ~VulkanDevice();

  [[nodiscard]] vk::Device get() const { return m_device; }
  [[nodiscard]] vk::PhysicalDevice getPhysical() const {
    return m_physicalDevice;
  }

  void waitIdle();

  [[nodiscard]] vk::Queue graphicsQueue() const { return m_graphicsQueue; }
  [[nodiscard]] vk::Queue computeQueue() const { return m_computeQueue; }
  [[nodiscard]] vk::Queue transferQueue() const { return m_transferQueue; }
  [[nodiscard]] vk::Queue presentQueue() const { return m_presentQueue; }

  [[nodiscard]] std::string name() const {
    return static_cast<const char*>(m_properties.deviceName);
  }
  [[nodiscard]] const QueueFamilyIndices& queueFamilies() const {
    return m_queueFamilyIndices;
  }

 private:
  vk::PhysicalDevice m_physicalDevice;
  vk::Device m_device;
  vk::PhysicalDeviceProperties m_properties;
  vk::PhysicalDeviceFeatures m_features;
  vk::PhysicalDeviceVulkan12Features m_features12;
  vk::PhysicalDeviceVulkan13Features m_features13;

  QueueFamilyIndices m_queueFamilyIndices;

  vk::Queue m_graphicsQueue;
  vk::Queue m_presentQueue;
  vk::Queue m_transferQueue;
  vk::Queue m_computeQueue;

  void pickPhysicalDevice(vk::Instance instance, vk::SurfaceKHR surface);
  void createDevice();
  void getQueues();

  static bool isDeviceSuitable(const vk::PhysicalDeviceProperties& properties,
                               const vk::PhysicalDeviceFeatures& features);

  bool findQueueFamilies(vk::SurfaceKHR surface);
};