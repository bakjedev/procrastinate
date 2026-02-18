#pragma once
#include <vector>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_structs.hpp>

class VulkanSwapChain
{
public:
  VulkanSwapChain(vk::Device device, vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface, vk::Extent2D extent);
  VulkanSwapChain(const VulkanSwapChain&) = delete;
  VulkanSwapChain(VulkanSwapChain&&) = delete;
  VulkanSwapChain& operator=(const VulkanSwapChain&) = delete;
  VulkanSwapChain& operator=(VulkanSwapChain&&) = delete;
  ~VulkanSwapChain();

  [[nodiscard]] vk::SwapchainKHR get() const { return swap_chain_; }
  [[nodiscard]] uint32_t imageCount() const { return image_count_; }
  [[nodiscard]] vk::Image getImage(const uint32_t imageIndex) const { return images_.at(imageIndex); }
  [[nodiscard]] const std::vector<vk::Image>& images() const { return images_; }
  [[nodiscard]] const std::vector<vk::ImageView>& imageViews() const { return image_views_; }

  [[nodiscard]] const vk::Extent2D& extent() const { return extent_; }

  [[nodiscard]] vk::Format format() const { return surface_format_.format; }

  vk::Result acquireNextImage(vk::Semaphore signalSemaphore, uint32_t& imageIndex);
  vk::Result present(uint32_t imageIndex, vk::Queue presentQueue, vk::Semaphore waitSemaphore) const;

  void recreate(vk::Extent2D extent);

private:
  vk::SwapchainKHR swap_chain_;

  vk::PhysicalDevice physical_device_;
  vk::SurfaceKHR surface_;
  vk::Device device_; // for deconstructing

  vk::SurfaceFormatKHR surface_format_;
  vk::PresentModeKHR present_mode_{};

  vk::Extent2D extent_;

  std::vector<vk::Image> images_;
  std::vector<vk::ImageView> image_views_;
  uint32_t image_count_{};
  uint32_t image_index_{};

  void Create();
  void Destroy() const;

  void GetImages();
  void CreateImageViews();

  vk::SurfaceCapabilitiesKHR GetCapabilities() const;
  void ChooseSurfaceFormat();
  void ChoosePresentMode();
};
