#include "render/vk_swap_chain.hpp"

#include "util/print.hpp"

VulkanSwapChain::VulkanSwapChain(const vk::Device device, const vk::PhysicalDevice physical_device,
                                 const vk::SurfaceKHR surface, const vk::Extent2D extent) :
    physical_device_(physical_device), surface_(surface), device_(device), extent_(extent)
{
  Create();
}

VulkanSwapChain::~VulkanSwapChain() { Destroy(); }

vk::Result VulkanSwapChain::AcquireNextImage(const vk::Semaphore signalSemaphore, uint32_t& imageIndex)
{
  const auto result = device_.acquireNextImageKHR(swap_chain_, UINT64_MAX, signalSemaphore, nullptr, &image_index_);
  imageIndex = image_index_;
  return result;
}

vk::Result VulkanSwapChain::Present(const uint32_t imageIndex, const vk::Queue presentQueue,
                                    const vk::Semaphore waitSemaphore) const
{
  vk::PresentInfoKHR present_info{};
  present_info.sType = vk::StructureType::ePresentInfoKHR;
  present_info.waitSemaphoreCount = 1;
  present_info.pWaitSemaphores = &waitSemaphore;
  present_info.swapchainCount = 1;
  present_info.pSwapchains = &swap_chain_;
  present_info.pImageIndices = &imageIndex;

  try
  {
    return presentQueue.presentKHR(present_info);
  } catch (const vk::OutOfDateKHRError&)
  {
    return vk::Result::eErrorOutOfDateKHR;
  }
}

void VulkanSwapChain::Recreate(const vk::Extent2D extent)
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
  uint32_t min_image_count = capabilities.minImageCount + 1;
  if (capabilities.maxImageCount > 0 && min_image_count > capabilities.maxImageCount)
  {
    min_image_count = capabilities.maxImageCount;
  }

  vk::SwapchainCreateInfoKHR create_info{};
  create_info.sType = vk::StructureType::eSwapchainCreateInfoKHR;
  create_info.surface = surface_;
  create_info.minImageCount = min_image_count;
  create_info.imageFormat = surface_format_.format;
  create_info.imageColorSpace = surface_format_.colorSpace;
  create_info.imageExtent = extent_;
  create_info.imageArrayLayers = 1;
  create_info.imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst;
  create_info.imageSharingMode = vk::SharingMode::eExclusive;
  create_info.preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity;
  create_info.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
  create_info.presentMode = present_mode_;
  create_info.clipped = VK_TRUE;

  swap_chain_ = device_.createSwapchainKHR(create_info);

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
    vk::ImageViewCreateInfo view_create_info{};
    view_create_info.sType = vk::StructureType::eImageViewCreateInfo;
    view_create_info.image = images_[i];
    view_create_info.viewType = vk::ImageViewType::e2D;
    view_create_info.format = surface_format_.format;
    view_create_info.components.r = vk::ComponentSwizzle::eIdentity;
    view_create_info.components.g = vk::ComponentSwizzle::eIdentity;
    view_create_info.components.b = vk::ComponentSwizzle::eIdentity;
    view_create_info.components.a = vk::ComponentSwizzle::eIdentity;
    view_create_info.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    view_create_info.subresourceRange.baseMipLevel = 0;
    view_create_info.subresourceRange.levelCount = 1;
    view_create_info.subresourceRange.baseArrayLayer = 0;
    view_create_info.subresourceRange.layerCount = 1;

    image_views_[i] = device_.createImageView(view_create_info);
  }
}

vk::SurfaceCapabilitiesKHR VulkanSwapChain::GetCapabilities() const
{
  return physical_device_.getSurfaceCapabilitiesKHR(surface_);
}

void VulkanSwapChain::ChooseSurfaceFormat()
{
  const auto surface_formats = physical_device_.getSurfaceFormatsKHR(surface_);

  if (surface_formats.empty())
  {
    throw std::runtime_error("Failed to find Vulkan surface formats");
  }

  for (const auto& surfaceFormat: surface_formats)
  {
    if (surfaceFormat.format == vk::Format::eB8G8R8A8Srgb &&
        surfaceFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) // prefer SRGB Non linear
    {
      surface_format_ = surfaceFormat;
      return;
    }
  }
  surface_format_ = surface_formats[0];
}

void VulkanSwapChain::ChoosePresentMode()
{
  const auto present_modes = physical_device_.getSurfacePresentModesKHR(surface_);

  if (present_modes.empty())
  {
    throw std::runtime_error("Failed to find present modes");
  }

  for (const auto& present_mode: present_modes)
  {
    if (present_mode == vk::PresentModeKHR::eMailbox) // prefer mailbox
    {
      present_mode_ = present_mode;
      return;
    } else if (present_mode == vk::PresentModeKHR::eFifo)
    {
      present_mode_ = present_mode;
      return;
    }
  }
  present_mode_ = vk::PresentModeKHR::eImmediate;
}
