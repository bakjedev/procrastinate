
#include "render/vk_frame.hpp"

#include <memory>
#include <vulkan/vulkan_structs.hpp>

#include "render/vk_command_pool.hpp"
#include "render/vk_descriptor.hpp"
#include "util/print.hpp"
#include "vk_allocator.hpp"
#include "vma/vma_usage.h"
#include "vulkan/vulkan.hpp"

#define MAX_OBJECTS 10000
#define MAX_LINES 10000

VulkanFrame::VulkanFrame(const VulkanCommandPool* graphicsPool,
                         const VulkanCommandPool* transferPool,
                         const VulkanCommandPool* computePool,
                         const VulkanDescriptorPool* descriptorPool,
                         const VulkanDescriptorSetLayout* descriptorLayout,
                         const vk::Device device,
                         const VulkanAllocator* allocator) {
  // Set references
  m_device = device;

  // Create per frame sync objects
  constexpr vk::SemaphoreCreateInfo semaphoreCreateInfo{};
  constexpr vk::FenceCreateInfo fenceCreateInfo{
      .flags = vk::FenceCreateFlagBits::eSignaled};

  m_imageAvailable = m_device.createSemaphoreUnique(semaphoreCreateInfo);
  m_inFlight = m_device.createFenceUnique(fenceCreateInfo);
  m_computeFinished = m_device.createSemaphoreUnique(semaphoreCreateInfo);

  // Allocate command buffers for graphics, transfer and compute.
  m_graphicsCmd = graphicsPool->allocate();
  m_transferCmd = transferPool->allocate();
  m_computeCmd = computePool->allocate();

  // Create render object buffer
  m_objectBuffer = std::make_unique<VulkanBuffer>(
      BufferInfo{
          .size = sizeof(RenderObject) * MAX_OBJECTS,
          .usage = vk::BufferUsageFlagBits::eStorageBuffer |
                   vk::BufferUsageFlagBits::eTransferDst,
          .memoryUsage = VMA_MEMORY_USAGE_AUTO,
          .memoryFlags = VMA_ALLOCATION_CREATE_MAPPED_BIT |
                         VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT

      },
      allocator->get());
  m_objectBuffer->map();

  // Create debug line vertex buffer
  m_debugLineVertexBuffer = std::make_unique<VulkanBuffer>(
      BufferInfo{
          .size = sizeof(DebugLineVertex) * 2 * MAX_LINES,
          .usage = vk::BufferUsageFlagBits::eVertexBuffer,
          .memoryUsage = VMA_MEMORY_USAGE_AUTO,
          .memoryFlags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
      },
      allocator->get()
  );
  m_debugLineVertexBuffer->map();

  // Create draw count buffer
  m_drawCount = std::make_unique<VulkanBuffer>(
      BufferInfo{.size = sizeof(uint32_t),
                 .usage = vk::BufferUsageFlagBits::eStorageBuffer |
                 vk::BufferUsageFlagBits::eTransferDst,
                 .memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY,
                 .memoryFlags = {}},
      allocator->get());

  
  // Allocate descriptor set with per frame descriptor set layout
  m_descriptorSet = descriptorPool->allocate(descriptorLayout->get());
}

VulkanFrame::~VulkanFrame() { m_objectBuffer->unmap();
m_debugLineVertexBuffer->unmap();}
