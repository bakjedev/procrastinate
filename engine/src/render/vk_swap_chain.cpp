#include "render/vk_swap_chain.hpp"

#include "util/print.hpp"

VulkanSwapChain::VulkanSwapChain(const vk::Device device, const vk::PhysicalDevice physicalDevice,
                                 const vk::SurfaceKHR surface, const vk::Extent2D extent) :
    physical_device_(physicalDevice), surface_(surface), device_(device), extent_(extent)
{
  Create();
}

VulkanSwapChain::~VulkanSwapChain() { Destroy(); }

vk::Result VulkanSwapChain::acquireNextImage(const vk::Semaphore signalSemaphore, uint32_t& imageIndex)
{
  const auto result = device_.acquireNextImageKHR(swap_chain_, UINT64_MAX, signalSemaphore, nullptr, &image_index_);
  imageIndex = image_index_;
  return result;
}

vk::Result VulkanSwapChain::present(const uint32_t imageIndex, const vk::Queue presentQueue,
                                    const vk::Semaphore waitSemaphore) const
{
  vk::PresentInfoKHR presentInfo{};
  presentInfo.sType = vk::StructureType::ePresentInfoKHR;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = &waitSemaphore;
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = &swap_chain_;
  presentInfo.pImageIndices = &imageIndex;

  try
  {
    return presentQueue.presentKHR(presentInfo);
  } catch (const vk::OutOfDateKHRError&)
  {
    return vk::Result::eErrorOutOfDateKHR;
  }
}

void VulkanSwapChain::recreate(vk::Extent2D extent)
{
  device_.waitIdle();
  Destroy();
  extent_ = extent;
  Create();
}

void VulkanSwapChain::Create()
{
  ChooseSurfaceFormat();
  ChoosePresentMode();

  const auto capabilities = GetCapabilities();

  if (extent_.width == 0 || extent_.height == 0)
  {
    extent_ = capabilities.currentExtent;
  }
  /* // check for invalid surface extent
  if (capabilities.currentExtent.width == UINT32_MAX ||
      capabilities.currentExtent.height == UINT32_MAX) {
    throw std::runtime_error("Failed to get Vulkan surface extent");
  }

  m_extent = capabilities.currentExtent; */

  // calculate the amount of image the swap chain will have
  uint32_t minImageCount = capabilities.minImageCount + 1;
  if (capabilities.maxImageCount > 0 && minImageCount > capabilities.maxImageCount)
  {
    minImageCount = capabilities.maxImageCount;
  }

  vk::SwapchainCreateInfoKHR createInfo{};
  createInfo.sType = vk::StructureType::eSwapchainCreateInfoKHR;
  createInfo.surface = surface_;
  createInfo.minImageCount = minImageCount;
  createInfo.imageFormat = surface_format_.format;
  createInfo.imageColorSpace = surface_format_.colorSpace;
  createInfo.imageExtent = extent_;
  createInfo.imageArrayLayers = 1;
  createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst;
  createInfo.imageSharingMode = vk::SharingMode::eExclusive;
  createInfo.preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity;
  createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
  createInfo.presentMode = present_mode_;
  createInfo.clipped = VK_TRUE;

  swap_chain_ = device_.createSwapchainKHR(createInfo);

  GetImages();
  CreateImageViews();
}

void VulkanSwapChain::Destroy() const
{
  for (const auto& view: image_views_)
  {
    device_.destroyImageView(view);
  }
  device_.destroySwapchainKHR(swap_chain_);
}

void VulkanSwapChain::GetImages()
{
  images_ = device_.getSwapchainImagesKHR(swap_chain_);
  image_count_ = static_cast<uint32_t>(images_.size());
}

void VulkanSwapChain::CreateImageViews()
{
  image_views_.resize(image_count_);
  for (size_t i = 0; i < image_count_; ++i)
  {
    vk::ImageViewCreateInfo viewCreateInfo{};
    viewCreateInfo.sType = vk::StructureType::eImageViewCreateInfo;
    viewCreateInfo.image = images_[i];
    viewCreateInfo.viewType = vk::ImageViewType::e2D;
    viewCreateInfo.format = surface_format_.format;
    viewCreateInfo.components.r = vk::ComponentSwizzle::eIdentity;
    viewCreateInfo.components.g = vk::ComponentSwizzle::eIdentity;
    viewCreateInfo.components.b = vk::ComponentSwizzle::eIdentity;
    viewCreateInfo.components.a = vk::ComponentSwizzle::eIdentity;
    viewCreateInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    viewCreateInfo.subresourceRange.baseMipLevel = 0;
    viewCreateInfo.subresourceRange.levelCount = 1;
    viewCreateInfo.subresourceRange.baseArrayLayer = 0;
    viewCreateInfo.subresourceRange.layerCount = 1;

    image_views_[i] = device_.createImageView(viewCreateInfo);
  }
}

vk::SurfaceCapabilitiesKHR VulkanSwapChain::GetCapabilities() const
{
  return physical_device_.getSurfaceCapabilitiesKHR(surface_);
}

void VulkanSwapChain::ChooseSurfaceFormat()
{
  const auto surfaceFormats = physical_device_.getSurfaceFormatsKHR(surface_);

  if (surfaceFormats.empty())
  {
    throw std::runtime_error("Failed to find Vulkan surface formats");
  }

  for (const auto& surfaceFormat: surfaceFormats)
  {
    if (surfaceFormat.format == vk::Format::eB8G8R8A8Srgb &&
        surfaceFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) // prefer SRGB Non linear
    {
      surface_format_ = surfaceFormat;
      return;
    }
  }
  surface_format_ = surfaceFormats[0];
}

void VulkanSwapChain::ChoosePresentMode()
{
  const auto presentModes = physical_device_.getSurfacePresentModesKHR(surface_);

  if (presentModes.empty())
  {
    throw std::runtime_error("Failed to find present modes");
  }

  for (const auto& presentMode: presentModes)
  {
    if (presentMode == vk::PresentModeKHR::eMailbox) // prefer mailbox
    {
      present_mode_ = presentMode;
      return;
    } else if (presentMode == vk::PresentModeKHR::eFifo)
    {
      present_mode_ = presentMode;
      return;
    }
  }
  present_mode_ = vk::PresentModeKHR::eImmediate;
}
