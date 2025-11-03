#pragma once
#include <vulkan/vulkan.h>

#include <vector>


class VulkanSwapChain {
 public:
  VulkanSwapChain(VkDevice device, VkPhysicalDevice physicalDevice,
                  VkSurfaceKHR surface);
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

  void recreate(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);

 private:
  VkSwapchainKHR m_swapChain{};

  VkSurfaceFormatKHR m_surfaceFormat{};
  VkPresentModeKHR m_presentMode{};

  std::vector<VkImage> m_images;
  std::vector<VkImageView> m_imageViews;
  uint32_t m_imageCount{};
  uint32_t m_imageIndex{};

  VkDevice m_device;  // for deconstructing

  void create(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
  void destroy();

  void getImages();
  void createImageViews();

  static VkSurfaceCapabilitiesKHR getCapabilities(
      VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
  void chooseSurfaceFormat(VkPhysicalDevice physicalDevice,
                           VkSurfaceKHR surface);
  void choosePresentMode(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
};