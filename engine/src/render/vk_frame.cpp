#include "render/vk_frame.hpp"

#include "render/vk_command_pool.hpp"
#include "util/print.hpp"

VulkanFrame::VulkanFrame(VulkanCommandPool* graphicsPool,
                         VulkanCommandPool* transferPool,
                         VulkanCommandPool* computePool, vk::Device device)
    : m_graphicsCmd(graphicsPool->allocate()),
      m_transferCmd(transferPool->allocate()),
      m_computeCmd(computePool->allocate()),
      m_device(device) {
  vk::SemaphoreCreateInfo semaphoreCreateInfo{};

  m_renderFinished = m_device.createSemaphore(semaphoreCreateInfo);

  Util::println("Created vulkan frame");
}

VulkanFrame::~VulkanFrame() {
  m_device.destroySemaphore(m_renderFinished);
  Util::println("Destroyed vulkan frame");
}
