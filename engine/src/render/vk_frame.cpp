
#include "render/vk_frame.hpp"

#include "render/vk_command_pool.hpp"
#include "render/vk_descriptor.hpp"
#include "render/vk_device.hpp"
#include "vk_allocator.hpp"
#include "vk_buffer.hpp"
#include "vk_image.hpp"
#include "vk_renderer.hpp"

constexpr uint32_t MAX_INDIRECT_COMMANDS = 65536;
constexpr uint32_t MAX_LINES = 10000;
constexpr uint32_t MAX_OBJECTS = 10000;

VulkanFrame::VulkanFrame(const VulkanCommandPool* graphics_pool, const VulkanCommandPool* compute_pool,
                         const VulkanDescriptorPool* descriptor_pool,
                         const VulkanDescriptorSetLayout* descriptor_layout, VulkanDevice* device,
                         VulkanAllocator* allocator) : device_(device), allocator_(allocator)
{
  // Create per frame sync objects
  constexpr vk::SemaphoreCreateInfo semaphoreCreateInfo{};
  constexpr vk::FenceCreateInfo fenceCreateInfo{.flags = vk::FenceCreateFlagBits::eSignaled};

  const auto internalDevice = device_->get();
  image_available_ = internalDevice.createSemaphoreUnique(semaphoreCreateInfo);
  in_flight_ = internalDevice.createFenceUnique(fenceCreateInfo);
  compute_finished_ = internalDevice.createSemaphoreUnique(semaphoreCreateInfo);

  // Allocate command buffers for graphics, transfer and compute.
  graphics_cmd_ = graphics_pool->allocate();
  compute_cmd_ = compute_pool->allocate();

  // Create render object buffer
  object_buffer_ = std::make_unique<VulkanBuffer>(
      BufferInfo{
          .size = sizeof(RenderObject) * MAX_OBJECTS,
          .usage = vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst,
          .memoryUsage = VMA_MEMORY_USAGE_AUTO,
          .memoryFlags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT

      },
      allocator->get(), device_);
  object_buffer_->map();

  indirect_buffer_ = std::make_unique<VulkanBuffer>(
      BufferInfo{.size = sizeof(vk::DrawIndexedIndirectCommand) * MAX_INDIRECT_COMMANDS,
                 .usage = vk::BufferUsageFlagBits::eIndirectBuffer | vk::BufferUsageFlagBits::eTransferDst |
                          vk::BufferUsageFlagBits::eStorageBuffer,
                 .memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY,
                 .memoryFlags = {}},
      allocator->get(), device_);

  // Create debug line vertex buffer
  debug_line_vertex_buffer_ =
      std::make_unique<VulkanBuffer>(BufferInfo{.size = sizeof(DebugLineVertex) * 2 * MAX_LINES,
                                                .usage = vk::BufferUsageFlagBits::eVertexBuffer,
                                                .memoryUsage = VMA_MEMORY_USAGE_AUTO,
                                                .memoryFlags = VMA_ALLOCATION_CREATE_MAPPED_BIT |
                                                               VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT},
                                     allocator->get(), device_);
  debug_line_vertex_buffer_->map();

  // Create draw count buffer
  draw_count_ = std::make_unique<VulkanBuffer>(
      BufferInfo{.size = sizeof(uint32_t),
                 .usage = vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst |
                          vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eIndirectBuffer,
                 .memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY,
                 .memoryFlags = {}},
      allocator->get(), device_);

  // Allocate descriptor set with per frame descriptor set layout
  descriptor_set_ = descriptor_pool->allocate(descriptor_layout->get());
}

VulkanFrame::~VulkanFrame()
{
  object_buffer_->unmap();
  debug_line_vertex_buffer_->unmap();
}
void VulkanFrame::RecreateFrameImages(const uint32_t width, const uint32_t height)
{
  depth_image_ = nullptr;

  const ImageInfo depthImageInfo{
      .width = width,
      .height = height,
      .format = vk::Format::eD32Sfloat,
      .usage = vk::ImageUsageFlagBits::eDepthStencilAttachment,
      .aspect_flags = vk::ImageAspectFlagBits::eDepth,
  };
  depth_image_ = std::make_unique<VulkanImage>(depthImageInfo, allocator_->get());

  visibility_image_ = nullptr;
  const ImageInfo visibilityImageInfo{
      .width = width,
      .height = height,
      .format = vk::Format::eR32Uint,
      .usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
      .aspect_flags = vk::ImageAspectFlagBits::eColor,
  };
  visibility_image_ = std::make_unique<VulkanImage>(visibilityImageInfo, allocator_->get());

  render_image_ = nullptr;
  const ImageInfo renderImageInfo{
      .width = width,
      .height = height,
      .format = vk::Format::eB8G8R8A8Unorm,
      .usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc |
               vk::ImageUsageFlagBits::eStorage,
      .aspect_flags = vk::ImageAspectFlagBits::eColor,
  };
  render_image_ = std::make_unique<VulkanImage>(renderImageInfo, allocator_->get());
}
