#include "render/vk_renderer.hpp"

#include <SDL3/SDL.h>

#include <memory>

#include "util/print.hpp"
#include "vk_allocator.hpp"

VulkanRenderer::VulkanRenderer(SDL_Window* window) {
  m_instance = std::make_unique<VulkanInstance>();

  m_surface = std::make_unique<VulkanSurface>(window, m_instance->get());

  m_physicalDevice = std::make_unique<VulkanPhysicalDevice>(m_instance->get(),
                                                            m_surface->get());

  m_logicalDevice = std::make_unique<VulkanLogicalDevice>(
      m_physicalDevice->get(), m_physicalDevice->features(),
      m_physicalDevice->queueFamilies(), m_physicalDevice->features12(),
      m_physicalDevice->features13());

  m_allocator = std::make_unique<VulkanAllocator>(
      m_physicalDevice->get(), m_logicalDevice->get(), m_instance->get());

  m_swapChain = std::make_unique<VulkanSwapChain>(
      m_logicalDevice->get(), m_physicalDevice->get(), m_surface->get());

  m_graphicsPool = std::make_unique<VulkanCommandPool>(
      m_logicalDevice->get(),
      m_physicalDevice->queueFamilies().graphics.value(), 0);
  m_transferPool = std::make_unique<VulkanCommandPool>(
      m_logicalDevice->get(),
      m_physicalDevice->queueFamilies().transfer.value(), 0);
  m_computePool = std::make_unique<VulkanCommandPool>(
      m_logicalDevice->get(), m_physicalDevice->queueFamilies().compute.value(),
      0);

  m_frameCount = m_swapChain->imageCount();
  m_frames.reserve(m_frameCount);
  for (uint32_t i = 0; i < m_frameCount; ++i) {
    m_frames.emplace_back(std::make_unique<VulkanFrame>(
        m_graphicsPool.get(), m_transferPool.get(), m_computePool.get(),
        m_logicalDevice->get()));
  }
  Util::println("Created vulkan renderer");
}

VulkanRenderer::~VulkanRenderer() {
  m_logicalDevice->waitIdle();
  Util::println("Destroyed vulkan renderer");
}