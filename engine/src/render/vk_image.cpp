#include "vk_image.hpp"

#include "util/vk_check.hpp"

VulkanImage::VulkanImage(const ImageInfo &info, const VmaAllocator allocator) :
    format_(info.format), width_(info.width), height_(info.height), allocator_(allocator)
{
  VmaAllocatorInfo allocator_info;
  vmaGetAllocatorInfo(allocator_, &allocator_info);
  device_ = allocator_info.device;

  Create(info);
  CreateView(info.aspect_flags);
}

VulkanImage::~VulkanImage() { Destroy(); }

void VulkanImage::TransitionLayout(const vk::CommandBuffer cmd, const vk::ImageLayout old_layout,
                                   const vk::ImageLayout new_layout) const
{
  TransitionImageLayout(image_, cmd, old_layout, new_layout);
}

// static
void VulkanImage::TransitionImageLayout(const vk::Image image, const vk::CommandBuffer cmd,
                                        const vk::ImageLayout old_layout, const vk::ImageLayout new_layout)
{
  const vk::ImageAspectFlags aspect_mask = (new_layout == vk::ImageLayout::eDepthAttachmentOptimal)
                                               ? vk::ImageAspectFlagBits::eDepth
                                               : vk::ImageAspectFlagBits::eColor;

  vk::ImageMemoryBarrier2 barrier{
      .oldLayout = old_layout,
      .newLayout = new_layout,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = image,
      .subresourceRange = vk::ImageSubresourceRange{.aspectMask = aspect_mask,
                                                    .baseMipLevel = 0,
                                                    .levelCount = VK_REMAINING_MIP_LEVELS,
                                                    .baseArrayLayer = 0,
                                                    .layerCount = VK_REMAINING_ARRAY_LAYERS}};

  if (old_layout == vk::ImageLayout::eUndefined && new_layout == vk::ImageLayout::eTransferDstOptimal)
  {
    barrier.srcAccessMask = vk::AccessFlagBits2::eNone;
    barrier.dstAccessMask = vk::AccessFlagBits2::eTransferWrite;
    barrier.srcStageMask = vk::PipelineStageFlagBits2::eTopOfPipe;
    barrier.dstStageMask = vk::PipelineStageFlagBits2::eTransfer;
  } else if (old_layout == vk::ImageLayout::eUndefined && new_layout == vk::ImageLayout::eTransferSrcOptimal)
  {
    barrier.srcAccessMask = vk::AccessFlagBits2::eNone;
    barrier.dstAccessMask = vk::AccessFlagBits2::eTransferRead;
    barrier.srcStageMask = vk::PipelineStageFlagBits2::eTopOfPipe;
    barrier.dstStageMask = vk::PipelineStageFlagBits2::eTransfer;
  } else if (old_layout == vk::ImageLayout::eTransferDstOptimal &&
             new_layout == vk::ImageLayout::eShaderReadOnlyOptimal)
  {
    barrier.srcAccessMask = vk::AccessFlagBits2::eTransferWrite;
    barrier.dstAccessMask = vk::AccessFlagBits2::eShaderRead | vk::AccessFlagBits2::eInputAttachmentRead;
    barrier.srcStageMask = vk::PipelineStageFlagBits2::eTransfer;
    barrier.dstStageMask = vk::PipelineStageFlagBits2::eComputeShader | vk::PipelineStageFlagBits2::eFragmentShader;
  } else if (old_layout == vk::ImageLayout::eUndefined && new_layout == vk::ImageLayout::eShaderReadOnlyOptimal)
  {
    barrier.srcAccessMask = vk::AccessFlagBits2::eNone;
    barrier.dstAccessMask = vk::AccessFlagBits2::eShaderRead | vk::AccessFlagBits2::eInputAttachmentRead;
    barrier.srcStageMask = vk::PipelineStageFlagBits2::eTopOfPipe;
    barrier.dstStageMask = vk::PipelineStageFlagBits2::eComputeShader | vk::PipelineStageFlagBits2::eFragmentShader;
  } else if (old_layout == vk::ImageLayout::eColorAttachmentOptimal &&
             new_layout == vk::ImageLayout::eTransferSrcOptimal)
  {
    barrier.srcAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite | vk::AccessFlagBits2::eColorAttachmentRead;
    barrier.dstAccessMask = vk::AccessFlagBits2::eTransferRead;
    barrier.srcStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
    barrier.dstStageMask = vk::PipelineStageFlagBits2::eTransfer;
  } else if (old_layout == vk::ImageLayout::eTransferSrcOptimal &&
             new_layout == vk::ImageLayout::eColorAttachmentOptimal)
  {
    barrier.srcAccessMask = vk::AccessFlagBits2::eTransferRead;
    barrier.dstAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite | vk::AccessFlagBits2::eColorAttachmentRead;
    barrier.srcStageMask = vk::PipelineStageFlagBits2::eTransfer;
    barrier.dstStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
  } else if (old_layout == vk::ImageLayout::eColorAttachmentOptimal &&
             new_layout == vk::ImageLayout::eShaderReadOnlyOptimal)
  {
    barrier.srcAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite | vk::AccessFlagBits2::eColorAttachmentRead;
    barrier.dstAccessMask = vk::AccessFlagBits2::eShaderRead | vk::AccessFlagBits2::eInputAttachmentRead;
    barrier.srcStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
    barrier.dstStageMask = vk::PipelineStageFlagBits2::eComputeShader | vk::PipelineStageFlagBits2::eFragmentShader;
  } else if (old_layout == vk::ImageLayout::eShaderReadOnlyOptimal &&
             new_layout == vk::ImageLayout::eColorAttachmentOptimal)
  {
    barrier.srcAccessMask = vk::AccessFlagBits2::eShaderRead | vk::AccessFlagBits2::eInputAttachmentRead;
    barrier.dstAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite | vk::AccessFlagBits2::eColorAttachmentRead;
    barrier.srcStageMask = vk::PipelineStageFlagBits2::eComputeShader | vk::PipelineStageFlagBits2::eFragmentShader;
    barrier.dstStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
  } else if (old_layout == vk::ImageLayout::eColorAttachmentOptimal && new_layout == vk::ImageLayout::eGeneral)
  {
    barrier.srcAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite | vk::AccessFlagBits2::eColorAttachmentRead;
    barrier.dstAccessMask = vk::AccessFlagBits2::eMemoryRead | vk::AccessFlagBits2::eMemoryWrite;
    barrier.srcStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
    barrier.dstStageMask = vk::PipelineStageFlagBits2::eComputeShader;
  } else if (old_layout == vk::ImageLayout::eTransferSrcOptimal && new_layout == vk::ImageLayout::eGeneral)
  {
    barrier.srcAccessMask = vk::AccessFlagBits2::eTransferRead;
    barrier.dstAccessMask = vk::AccessFlagBits2::eMemoryRead | vk::AccessFlagBits2::eMemoryWrite;
    barrier.srcStageMask = vk::PipelineStageFlagBits2::eTransfer;
    barrier.dstStageMask = vk::PipelineStageFlagBits2::eComputeShader;
  } else if (old_layout == vk::ImageLayout::eGeneral && new_layout == vk::ImageLayout::eColorAttachmentOptimal)
  {
    barrier.srcAccessMask = vk::AccessFlagBits2::eMemoryRead | vk::AccessFlagBits2::eMemoryWrite;
    barrier.dstAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite | vk::AccessFlagBits2::eColorAttachmentRead;
    barrier.srcStageMask = vk::PipelineStageFlagBits2::eComputeShader;
    barrier.dstStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
  } else if (old_layout == vk::ImageLayout::eUndefined && new_layout == vk::ImageLayout::eDepthStencilAttachmentOptimal)
  {
    barrier.srcAccessMask = vk::AccessFlagBits2::eNone;
    barrier.dstAccessMask =
        vk::AccessFlagBits2::eDepthStencilAttachmentRead | vk::AccessFlagBits2::eDepthStencilAttachmentWrite;
    barrier.srcStageMask = vk::PipelineStageFlagBits2::eTopOfPipe;
    barrier.dstStageMask = vk::PipelineStageFlagBits2::eEarlyFragmentTests;
  } else if (old_layout == vk::ImageLayout::eUndefined && new_layout == vk::ImageLayout::eColorAttachmentOptimal)
  {
    barrier.srcAccessMask = vk::AccessFlagBits2::eNone;
    barrier.dstAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite;
    barrier.srcStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
    barrier.dstStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
  } else if (old_layout == vk::ImageLayout::eColorAttachmentOptimal && new_layout == vk::ImageLayout::ePresentSrcKHR)
  {
    barrier.srcAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite | vk::AccessFlagBits2::eColorAttachmentRead;
    barrier.dstAccessMask = vk::AccessFlagBits2::eNone;
    barrier.srcStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
    barrier.dstStageMask = vk::PipelineStageFlagBits2::eBottomOfPipe;
  } else if (old_layout == vk::ImageLayout::eUndefined && new_layout == vk::ImageLayout::ePresentSrcKHR)
  {
    barrier.srcAccessMask = vk::AccessFlagBits2::eNone;
    barrier.dstAccessMask = vk::AccessFlagBits2::eNone;
    barrier.srcStageMask = vk::PipelineStageFlagBits2::eNone;
    barrier.dstStageMask = vk::PipelineStageFlagBits2::eBottomOfPipe;
  } else if (old_layout == vk::ImageLayout::ePresentSrcKHR && new_layout == vk::ImageLayout::eTransferDstOptimal)
  {
    barrier.srcAccessMask = vk::AccessFlagBits2::eNone;
    barrier.dstAccessMask = vk::AccessFlagBits2::eTransferWrite;
    barrier.srcStageMask = vk::PipelineStageFlagBits2::eBottomOfPipe;
    barrier.dstStageMask = vk::PipelineStageFlagBits2::eTransfer;
  } else if (old_layout == vk::ImageLayout::eTransferDstOptimal && new_layout == vk::ImageLayout::ePresentSrcKHR)
  {
    barrier.srcAccessMask = vk::AccessFlagBits2::eTransferWrite;
    barrier.dstAccessMask = vk::AccessFlagBits2::eNone;
    barrier.srcStageMask = vk::PipelineStageFlagBits2::eTransfer;
    barrier.dstStageMask = vk::PipelineStageFlagBits2::eBottomOfPipe;
  } else
  {
    throw std::runtime_error("Unsupported layout transition");
  }

  const vk::DependencyInfo dep_info{.imageMemoryBarrierCount = 1, .pImageMemoryBarriers = &barrier};

  cmd.pipelineBarrier2(dep_info);
}

void VulkanImage::Destroy()
{
  if (image_view_)
  {
    device_.destroyImageView(image_view_);
    image_view_ = nullptr;
  }
  if (image_)
  {
    vmaDestroyImage(allocator_, static_cast<VkImage>(image_), allocation_);
    image_ = nullptr;
    allocation_ = VK_NULL_HANDLE;
  }
}

void VulkanImage::Create(const ImageInfo &info)
{
  vk::ImageCreateInfo image_info{.imageType = vk::ImageType::e2D,
                                 .format = info.format,
                                 .extent = vk::Extent3D{.width = info.width, .height = info.height, .depth = 1},
                                 .mipLevels = 1,
                                 .arrayLayers = 1,
                                 .samples = vk::SampleCountFlagBits::e1,
                                 .tiling = vk::ImageTiling::eOptimal,
                                 .usage = info.usage,
                                 .sharingMode = vk::SharingMode::eExclusive,
                                 .initialLayout = vk::ImageLayout::eUndefined};

  VmaAllocationCreateInfo alloc_info{};
  alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
  alloc_info.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

  const auto raw_image_info = static_cast<VkImageCreateInfo>(image_info);

  VkImage temp_image = VK_NULL_HANDLE;
  VK_CHECK(vmaCreateImage(allocator_, &raw_image_info, &alloc_info, &temp_image, &allocation_, nullptr));
  image_ = temp_image;
}

void VulkanImage::CreateView(const vk::ImageAspectFlags aspect_flags)
{
  const vk::ImageViewCreateInfo view_info{
      .image = image_,
      .viewType = vk::ImageViewType::e2D,
      .format = format_,
      .subresourceRange = vk::ImageSubresourceRange{
          .aspectMask = aspect_flags, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1}};

  image_view_ = device_.createImageView(view_info);
}
