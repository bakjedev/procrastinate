#include "render/vk_swap_chain.hpp"

#include "util/print.hpp"

VulkanSwapChain::VulkanSwapChain(vk::Device device,
                                 vk::PhysicalDevice physicalDevice,
                                 vk::SurfaceKHR surface)
    : m_physicalDevice(physicalDevice), m_surface(surface), m_device(device) {
  create();
}

VulkanSwapChain::~VulkanSwapChain() { destroy(); }

vk::Result VulkanSwapChain::acquireNextImage(vk::Semaphore signalSemaphore,
                                             uint32_t& imageIndex) {
  const auto result = m_device.acquireNextImageKHR(
      m_swapChain, UINT64_MAX, signalSemaphore, nullptr, &m_imageIndex);
  imageIndex = m_imageIndex;
  return result;
}

vk::Result VulkanSwapChain::present(const uint32_t imageIndex,
                                    vk::Queue presentQueue,
                                    vk::Semaphore waitSemaphore) const {
  vk::PresentInfoKHR presentInfo{};
  presentInfo.sType = vk::StructureType::ePresentInfoKHR;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = &waitSemaphore;
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = &m_swapChain;
  presentInfo.pImageIndices = &imageIndex;

  return presentQueue.presentKHR(presentInfo);
}

void VulkanSwapChain::create() {
  chooseSurfaceFormat();
  choosePresentMode();

  const auto capabilities = getCapabilities();

  // check for invalid surface extent
  if (capabilities.currentExtent.width == UINT32_MAX ||
      capabilities.currentExtent.height == UINT32_MAX) {
    throw std::runtime_error("Failed to get Vulkan surface extent");
  }

  m_extent = capabilities.currentExtent;

  // calculate the amount of image the swap chain will have
  uint32_t minImageCount = capabilities.minImageCount + 1;
  if (capabilities.maxImageCount > 0 &&
      minImageCount > capabilities.maxImageCount) {
    minImageCount = capabilities.maxImageCount;
  }

  vk::SwapchainCreateInfoKHR createInfo{};
  createInfo.sType = vk::StructureType::eSwapchainCreateInfoKHR;
  createInfo.surface = m_surface;
  createInfo.minImageCount = minImageCount;
  createInfo.imageFormat = m_surfaceFormat.format;
  createInfo.imageColorSpace = m_surfaceFormat.colorSpace;
  createInfo.imageExtent = capabilities.currentExtent;
  createInfo.imageArrayLayers = 1;
  createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
  createInfo.imageSharingMode = vk::SharingMode::eExclusive;
  createInfo.preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity;
  createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
  createInfo.presentMode = m_presentMode;
  createInfo.clipped = VK_TRUE;

  m_swapChain = m_device.createSwapchainKHR(createInfo);

  getImages();
  createImageViews();

  Util::println(
      "Created vulkan swap chain, retrieved images, and created image views");
}

void VulkanSwapChain::destroy() {
  for (const auto& view : m_imageViews) {
    m_device.destroyImageView(view);
  }
  m_device.destroySwapchainKHR(m_swapChain);
  Util::println("Destroyed vulkan swap chain");
}

void VulkanSwapChain::recreate() {
  vkDeviceWaitIdle(m_device);
  destroy();
  create();
}

void VulkanSwapChain::getImages() {
  m_images = m_device.getSwapchainImagesKHR(m_swapChain);
  m_imageCount = static_cast<uint32_t>(m_images.size());
}

void VulkanSwapChain::createImageViews() {
  m_imageViews.resize(m_imageCount);
  for (size_t i = 0; i < m_imageCount; ++i) {
    vk::ImageViewCreateInfo viewCreateInfo{};
    viewCreateInfo.sType = vk::StructureType::eImageViewCreateInfo;
    viewCreateInfo.image = m_images[i];
    viewCreateInfo.viewType = vk::ImageViewType::e2D;
    viewCreateInfo.format = m_surfaceFormat.format;
    viewCreateInfo.components.r = vk::ComponentSwizzle::eIdentity;
    viewCreateInfo.components.g = vk::ComponentSwizzle::eIdentity;
    viewCreateInfo.components.b = vk::ComponentSwizzle::eIdentity;
    viewCreateInfo.components.a = vk::ComponentSwizzle::eIdentity;
    viewCreateInfo.subresourceRange.aspectMask =
        vk::ImageAspectFlagBits::eColor;
    viewCreateInfo.subresourceRange.baseMipLevel = 0;
    viewCreateInfo.subresourceRange.levelCount = 1;
    viewCreateInfo.subresourceRange.baseArrayLayer = 0;
    viewCreateInfo.subresourceRange.layerCount = 1;

    m_imageViews[i] = m_device.createImageView(viewCreateInfo);
  }
}

vk::SurfaceCapabilitiesKHR VulkanSwapChain::getCapabilities() {
  return m_physicalDevice.getSurfaceCapabilitiesKHR(m_surface);
}

void VulkanSwapChain::chooseSurfaceFormat() {
  auto surfaceFormats = m_physicalDevice.getSurfaceFormatsKHR(m_surface);

  if (surfaceFormats.empty()) {
    throw std::runtime_error("Failed to find Vulkan surface formats");
  }

  for (const auto& surfaceFormat : surfaceFormats) {
    if (surfaceFormat.format == vk::Format::eB8G8R8A8Srgb &&
        surfaceFormat.colorSpace ==
            vk::ColorSpaceKHR::eSrgbNonlinear)  // prefer SRGB Non linear
    {
      m_surfaceFormat = surfaceFormat;
      return;
    }
  }
  m_surfaceFormat = surfaceFormats[0];
}

void VulkanSwapChain::choosePresentMode() {
  auto presentModes = m_physicalDevice.getSurfacePresentModesKHR(m_surface);

  if (presentModes.empty()) {
    throw std::runtime_error("Failed to find present modes");
  }

  for (const auto& presentMode : presentModes) {
    if (presentMode == vk::PresentModeKHR::eMailbox)  // prefer mailbox
    {
      m_presentMode = presentMode;
      return;
    }
  }
  m_presentMode = vk::PresentModeKHR::eFifo;
}
