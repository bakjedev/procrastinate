
#include "render/vk_frame.hpp"

#include <memory>
#include <vulkan/vulkan_structs.hpp>

#include "render/vk_command_pool.hpp"
#include "util/print.hpp"
#include "vk_allocator.hpp"
#include "vma/vma_usage.h"
#include "vulkan/vulkan.hpp"
#include "render/vk_descriptor.hpp"

#define MAX_INDIRECT_COMMANDS 65536
#define MAX_OBJECTS 10000

VulkanFrame::VulkanFrame(VulkanCommandPool* graphicsPool,
                         VulkanCommandPool* transferPool,
                         VulkanCommandPool* computePool,
                         VulkanDescriptorPool* descriptorPool,
                         VulkanDescriptorSetLayout* descriptorLayout,
                         vk::Device device,
                         VulkanAllocator* allocator)
{
    m_device = device;
    
    vk::SemaphoreCreateInfo semaphoreCreateInfo{};
    m_renderFinished = m_device.createSemaphoreUnique(semaphoreCreateInfo);
    m_computeFinished = m_device.createSemaphoreUnique(semaphoreCreateInfo);

    Util::println("Created frame sync objects");

    m_graphicsCmd = graphicsPool->allocate();
    m_transferCmd = transferPool->allocate();
    m_computeCmd = computePool->allocate();
    
    Util::println("Allocated frame command buffers");

    m_indirectBuffer = std::make_unique<VulkanBuffer>(
        BufferInfo{
            .size = sizeof(vk::DrawIndexedIndirectCommand) * MAX_INDIRECT_COMMANDS,
            .usage = vk::BufferUsageFlagBits::eIndirectBuffer |
            vk::BufferUsageFlagBits::eTransferDst |
            vk::BufferUsageFlagBits::eStorageBuffer,
            .memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY,
            .memoryFlags = {}
        },
        allocator->get() 
    );
    
    m_objectBuffer = std::make_unique<VulkanBuffer>(
        BufferInfo{
            .size = sizeof(RenderObject) * MAX_OBJECTS,
            .usage = vk::BufferUsageFlagBits::eStorageBuffer |
            vk::BufferUsageFlagBits::eTransferDst |
            vk::BufferUsageFlagBits::eStorageBuffer,
            .memoryUsage = VMA_MEMORY_USAGE_AUTO,
            .memoryFlags = VMA_ALLOCATION_CREATE_MAPPED_BIT |
                           VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT

        },
        allocator->get() 
    );

    m_objectBuffer->map();
    
    Util::println("Created frame indirect buffer");

    m_descriptorSet = descriptorPool->allocate(descriptorLayout->get());

    Util::println("Allocated frame descriptor set");

    Util::println("Created vulkan frame");
}

VulkanFrame::~VulkanFrame() {
    m_objectBuffer->unmap();
    Util::println("Destroyed vulkan frame");
}
