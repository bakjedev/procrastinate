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

  [[nodiscard]] vk::SwapchainKHR get() const { return m_swapChain; }
  [[nodiscard]] uint32_t imageCount() const { return m_imageCount; }
  [[nodiscard]] vk::Image getImage(const uint32_t imageIndex) const { return m_images.at(imageIndex); }
  [[nodiscard]] const std::vector<vk::Image>& images() const { return m_images; }
  [[nodiscard]] const std::vector<vk::ImageView>& imageViews() const { return m_imageViews; }

  [[nodiscard]] const vk::Extent2D& extent() const { return m_extent; }

  [[nodiscard]] vk::Format format() const { return m_surfaceFormat.format; }

  vk::Result acquireNextImage(vk::Semaphore signalSemaphore, uint32_t& imageIndex);
  vk::Result present(uint32_t imageIndex, vk::Queue presentQueue, vk::Semaphore waitSemaphore) const;

  void recreate(vk::Extent2D extent);

private:
  vk::SwapchainKHR m_swapChain;

  vk::PhysicalDevice m_physicalDevice;
  vk::SurfaceKHR m_surface;
  vk::Device m_device; // for deconstructing

  vk::SurfaceFormatKHR m_surfaceFormat;
  vk::PresentModeKHR m_presentMode{};

  vk::Extent2D m_extent;

  std::vector<vk::Image> m_images;
  std::vector<vk::ImageView> m_imageViews;
  uint32_t m_imageCount{};
  uint32_t m_imageIndex{};

  void create();
  void destroy() const;

  void getImages();
  void createImageViews();

  vk::SurfaceCapabilitiesKHR getCapabilities() const;
  void chooseSurfaceFormat();
  void choosePresentMode();
};
