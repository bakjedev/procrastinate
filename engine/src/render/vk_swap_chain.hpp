#pragma once
#include <vulkan/vulkan.h>

#include <vector>

class VulkanSwapChain {
 public:
  VulkanSwapChain(VkDevice device, VkPhysicalDevice physicalDevice,
                  VkSurfaceKHR surface);
  VulkanSwapChain(const VulkanSwapChain&) = delete;
  VulkanSwapChain(VulkanSwapChain&&) = delete;
  VulkanSwapChain& operator=(const VulkanSwapChain&) = delete;
  VulkanSwapChain& operator=(VulkanSwapChain&&) = delete;
  ~VulkanSwapChain();

  [[nodiscard]] VkSwapchainKHR get() const { return m_swapChain; }
  [[nodiscard]] uint32_t imageCount() const { return m_imageCount; }
  [[nodiscard]] VkImage getImage(uint32_t imageIndex) const {
    return m_images.at(imageIndex);
  }
  [[nodiscard]] const std::vector<VkImage>& images() const { return m_images; }

  VkResult acquireNextImage(VkSemaphore signalSemaphore, uint32_t& imageIndex);
  VkResult present(uint32_t imageIndex, VkQueue presentQueue,
                   VkSemaphore waitSemaphore) const;

  void recreate();

 private:
  VkSwapchainKHR m_swapChain{};

  VkPhysicalDevice m_physicalDevice;
  VkSurfaceKHR m_surface;
  VkDevice m_device;  // for deconstructing

  VkSurfaceFormatKHR m_surfaceFormat{};
  VkPresentModeKHR m_presentMode{};

  std::vector<VkImage> m_images;
  std::vector<VkImageView> m_imageViews;
  uint32_t m_imageCount{};
  uint32_t m_imageIndex{};

  void create();
  void destroy();

  void getImages();
  void createImageViews();

  VkSurfaceCapabilitiesKHR getCapabilities();
  void chooseSurfaceFormat();
  void choosePresentMode();
};