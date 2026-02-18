#pragma once
#include <optional>
#include <string>
#include <vulkan/vulkan.hpp>

struct QueueFamilyIndices
{
  std::optional<uint32_t> graphics;
  std::optional<uint32_t> compute;
  std::optional<uint32_t> transfer;
  std::optional<uint32_t> present;

  [[nodiscard]] bool IsComplete() const
  {
    return graphics.has_value() && present.has_value() && compute.has_value() && transfer.has_value();
  }
};

class VulkanDevice
{
public:
  explicit VulkanDevice(vk::Instance instance, vk::SurfaceKHR surface);
  VulkanDevice(const VulkanDevice&) = delete;
  VulkanDevice(VulkanDevice&&) = delete;
  VulkanDevice& operator=(const VulkanDevice&) = delete;
  VulkanDevice& operator=(VulkanDevice&&) = delete;
  ~VulkanDevice();

  [[nodiscard]] vk::Device get() const { return device_; }
  [[nodiscard]] vk::PhysicalDevice GetPhysical() const { return physical_device_; }

  void WaitIdle() const;

  [[nodiscard]] vk::Queue GraphicsQueue() const { return graphics_queue_; }
  [[nodiscard]] vk::Queue ComputeQueue() const { return compute_queue_; }
  [[nodiscard]] vk::Queue TransferQueue() const { return transfer_queue_; }
  [[nodiscard]] vk::Queue PresentQueue() const { return present_queue_; }

  [[nodiscard]] std::string name() const { return static_cast<const char*>(properties_.deviceName); }
  [[nodiscard]] const QueueFamilyIndices& QueueFamilies() const { return queue_family_indices_; }
  [[nodiscard]] bool SeparateComputeQueue() const
  {
    return queue_family_indices_.compute.value() != queue_family_indices_.graphics.value();
  }

private:
  vk::PhysicalDevice physical_device_;
  vk::Device device_;
  vk::PhysicalDeviceProperties properties_;

  vk::PhysicalDeviceFeatures2 available_features_;
  vk::PhysicalDeviceVulkan11Features available_features11_;
  vk::PhysicalDeviceVulkan12Features available_features12_;
  vk::PhysicalDeviceVulkan13Features available_features13_;

  vk::PhysicalDeviceFeatures2 enabled_features_;
  vk::PhysicalDeviceVulkan11Features enabled_features11_;
  vk::PhysicalDeviceVulkan12Features enabled_features12_;
  vk::PhysicalDeviceVulkan13Features enabled_features13_;

  QueueFamilyIndices queue_family_indices_;

  vk::Queue graphics_queue_;
  vk::Queue present_queue_;
  vk::Queue transfer_queue_;
  vk::Queue compute_queue_;

  void PickPhysicalDevice(vk::Instance instance, vk::SurfaceKHR surface);
  void CreateDevice();
  void GetQueues();

  static bool IsDeviceSuitable(const vk::PhysicalDeviceProperties& properties);

  bool FindQueueFamilies(vk::SurfaceKHR surface);
};
