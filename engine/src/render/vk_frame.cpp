
#include "render/vk_frame.hpp"

#include <memory>

#include "render/vk_command_pool.hpp"
#include "util/print.hpp"
#include "vk_allocator.hpp"
#include "vma/vma_usage.h"
#include "vulkan/vulkan.hpp"

#define MAX_INDIRECT_COMMANDS 65536

VulkanFrame::VulkanFrame(VulkanCommandPool* graphicsPool,
                         VulkanCommandPool* transferPool,
                         VulkanCommandPool* computePool, vk::Device device,
                         VulkanAllocator* allocator)
    : m_graphicsCmd(graphicsPool->allocate()),
      m_transferCmd(transferPool->allocate()),
      m_computeCmd(computePool->allocate()),
      m_indirectBuffer(std::make_unique<VulkanBuffer>(
          BufferInfo{.size = sizeof(vk::DrawIndexedIndirectCommand) *
                             MAX_INDIRECT_COMMANDS,
                     .usage = vk::BufferUsageFlagBits::eIndirectBuffer |
                              vk::BufferUsageFlagBits::eTransferDst /* |
                               vk::BufferUsageFlagBits::eStorageBuffer // for
                               compute write*/
                     ,
                     .memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY,
                     .memoryFlags = VMA_ALLOCATION_CREATE_MAPPED_BIT},
          allocator->get())),
      m_stagingBuffer(std::make_unique<VulkanBuffer>(
          BufferInfo{
              .size = sizeof(vk::DrawIndexedIndirectCommand) *
                      MAX_INDIRECT_COMMANDS,
              .usage =
                  vk::BufferUsageFlagBits::eIndirectBuffer |
                  vk::BufferUsageFlagBits::eTransferSrc /* |
                  vk::BufferUsageFlagBits::eStorageBuffer // for compute write*/
              ,
              .memoryUsage = VMA_MEMORY_USAGE_CPU_TO_GPU,
              .memoryFlags = VMA_ALLOCATION_CREATE_MAPPED_BIT},
          allocator->get())),
      m_device(device) {
  vk::SemaphoreCreateInfo semaphoreCreateInfo{};

  m_renderFinished = m_device.createSemaphoreUnique(semaphoreCreateInfo);
  m_computeFinished = m_device.createSemaphoreUnique(semaphoreCreateInfo);

  Util::println("Created vulkan frame");
}

VulkanFrame::~VulkanFrame() {
  Util::println("Destroyed vulkan frame");
}
