#include "render/vk_swap_chain.hpp"

#include "util/util.hpp"

VulkanSwapChain::VulkanSwapChain(VkDevice device,
                                 VkPhysicalDevice physicalDevice,
                                 VkSurfaceKHR surface)
    : m_device(device) {
  create(physicalDevice, surface);
}

VulkanSwapChain::~VulkanSwapChain() { destroy(); }

VkResult VulkanSwapChain::acquireNextImage(VkSemaphore signalSemaphore,
                                           uint32_t& imageIndex) {
  const auto result =
      vkAcquireNextImageKHR(m_device, m_swapChain, UINT64_MAX, signalSemaphore,
                            VK_NULL_HANDLE, &m_imageIndex);
  imageIndex = m_imageIndex;
  return result;
}

VkResult VulkanSwapChain::present(const uint32_t imageIndex,
                                  VkQueue presentQueue,
                                  VkSemaphore waitSemaphore) const {
  VkPresentInfoKHR presentInfo{};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = &waitSemaphore;
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = &m_swapChain;
  presentInfo.pImageIndices = &imageIndex;

  return vkQueuePresentKHR(presentQueue, &presentInfo);
}

void VulkanSwapChain::create(VkPhysicalDevice physicalDevice,
                             VkSurfaceKHR surface) {
  chooseSurfaceFormat(physicalDevice, surface);
  choosePresentMode(physicalDevice, surface);

  const auto capabilities = getCapabilities(physicalDevice, surface);

  // check for invalid surface extent
  if (capabilities.currentExtent.width == UINT32_MAX ||
      capabilities.currentExtent.height == UINT32_MAX) {
    throw std::runtime_error("Failed to get Vulkan surface extent");
  }

  // calculate the amount of image the swap chain will have
  uint32_t minImageCount = capabilities.minImageCount + 1;
  if (capabilities.maxImageCount > 0 &&
      minImageCount > capabilities.maxImageCount) {
    minImageCount = capabilities.maxImageCount;
  }

  VkSwapchainCreateInfoKHR createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  createInfo.surface = surface;
  createInfo.minImageCount = minImageCount;
  createInfo.imageFormat = m_surfaceFormat.format;
  createInfo.imageColorSpace = m_surfaceFormat.colorSpace;
  createInfo.imageExtent = capabilities.currentExtent;
  createInfo.imageArrayLayers = 1;
  createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  createInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
  createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  createInfo.presentMode = m_presentMode;
  createInfo.clipped = VK_TRUE;

  VK_CHECK(vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swapChain));

  getImages();
  createImageViews();

  Util::println(
      "Created vulkan swap chain, retrieved images, and created image views");
}

void VulkanSwapChain::destroy() {
  for (auto* const view : m_imageViews) {
    vkDestroyImageView(m_device, view, nullptr);
  }
  vkDestroySwapchainKHR(m_device, m_swapChain, nullptr);
  Util::println("Destroyed vulkan swap chain");
}

void VulkanSwapChain::recreate(VkPhysicalDevice physicalDevice,
                               VkSurfaceKHR surface) {
  vkDeviceWaitIdle(m_device);
  destroy();
  create(physicalDevice, surface);
}

void VulkanSwapChain::getImages() {
  VK_CHECK(
      vkGetSwapchainImagesKHR(m_device, m_swapChain, &m_imageCount, nullptr));
  m_images.resize(m_imageCount);
  VK_CHECK(vkGetSwapchainImagesKHR(m_device, m_swapChain, &m_imageCount,
                                   m_images.data()));
}

void VulkanSwapChain::createImageViews() {
  m_imageViews.resize(m_imageCount);
  for (size_t i = 0; i < m_imageCount; ++i) {
    VkImageViewCreateInfo viewCreateInfo{};
    viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewCreateInfo.image = m_images[i];
    viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewCreateInfo.format = m_surfaceFormat.format;
    viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewCreateInfo.subresourceRange.baseMipLevel = 0;
    viewCreateInfo.subresourceRange.levelCount = 1;
    viewCreateInfo.subresourceRange.baseArrayLayer = 0;
    viewCreateInfo.subresourceRange.layerCount = 1;

    VK_CHECK(vkCreateImageView(m_device, &viewCreateInfo, nullptr,
                               &m_imageViews[i]));
  }
}

VkSurfaceCapabilitiesKHR VulkanSwapChain::getCapabilities(
    VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) {
  VkSurfaceCapabilitiesKHR capabilities;
  VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface,
                                                     &capabilities));
  return capabilities;
}

void VulkanSwapChain::chooseSurfaceFormat(VkPhysicalDevice physicalDevice,
                                          VkSurfaceKHR surface) {
  uint32_t surfaceFormatCount = 0;
  VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface,
                                                &surfaceFormatCount, nullptr));
  if (surfaceFormatCount == 0) {
    throw std::runtime_error("Failed to find Vulkan surface formats");
  }
  std::vector<VkSurfaceFormatKHR> surfaceFormats(surfaceFormatCount);
  VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(
      physicalDevice, surface, &surfaceFormatCount, surfaceFormats.data()));

  for (const auto& surfaceFormat : surfaceFormats) {
    if (surfaceFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
        surfaceFormat.colorSpace ==
            VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)  // prefer SRGB Non linear
    {
      m_surfaceFormat = surfaceFormat;
      return;
    }
  }
  m_surfaceFormat = surfaceFormats[0];
}

void VulkanSwapChain::choosePresentMode(VkPhysicalDevice physicalDevice,
                                        VkSurfaceKHR surface) {
  uint32_t presentModeCount = 0;
  VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(
      physicalDevice, surface, &presentModeCount, nullptr));
  if (presentModeCount == 0) {
    throw std::runtime_error("Failed to find Vulkan present modes");
  }
  std::vector<VkPresentModeKHR> presentModes(presentModeCount);
  VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(
      physicalDevice, surface, &presentModeCount, presentModes.data()));

  for (const auto& presentMode : presentModes) {
    if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)  // prefer mailbox
    {
      m_presentMode = presentMode;
      return;
    }
  }
  m_presentMode = VK_PRESENT_MODE_FIFO_KHR;
}
