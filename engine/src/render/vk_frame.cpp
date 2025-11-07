#include "render/vk_frame.hpp"

#include "render/vk_command_pool.hpp"
#include "util/vk_check.hpp"

VulkanFrame::VulkanFrame(VulkanCommandPool* graphicsPool,
                         VulkanCommandPool* transferPool,
                         VulkanCommandPool* computePool, VkDevice device)
    : m_graphicsCmd(graphicsPool->allocate()),
      m_transferCmd(transferPool->allocate()),
      m_computeCmd(computePool->allocate()),
      m_device(device) {
  VkSemaphoreCreateInfo semaphoreCreateInfo{
      VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, nullptr, 0U};
  VkFenceCreateInfo fenceCreateInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                                    nullptr, VK_FENCE_CREATE_SIGNALED_BIT};

  VK_CHECK(vkCreateSemaphore(m_device, &semaphoreCreateInfo, nullptr,
                             &m_imageAvailable));
  VK_CHECK(vkCreateSemaphore(m_device, &semaphoreCreateInfo, nullptr,
                             &m_renderFinished));
  VK_CHECK(vkCreateFence(m_device, &fenceCreateInfo, nullptr, &m_inFlight));

  Util::println("Created vulkan frame");
}

VulkanFrame::~VulkanFrame() {
  vkDestroySemaphore(m_device, m_imageAvailable, nullptr);
  vkDestroySemaphore(m_device, m_renderFinished, nullptr);
  vkDestroyFence(m_device, m_inFlight, nullptr);
  Util::println("Destroyed vulkan frame");
}