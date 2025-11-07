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

  const CommandPoolInfo graphicsPoolInfo{
      .queueFamilyIndex = m_physicalDevice->queueFamilies().graphics.value(),
      .flags = 0};
  m_graphicsPool = std::make_unique<VulkanCommandPool>(graphicsPoolInfo,
                                                       m_logicalDevice->get());

  const CommandPoolInfo transferPoolInfo{
      .queueFamilyIndex = m_physicalDevice->queueFamilies().transfer.value(),
      .flags = 0};
  m_transferPool = std::make_unique<VulkanCommandPool>(transferPoolInfo,
                                                       m_logicalDevice->get());

  const CommandPoolInfo computePoolInfo{
      .queueFamilyIndex = m_physicalDevice->queueFamilies().compute.value(),
      .flags = 0};
  m_computePool = std::make_unique<VulkanCommandPool>(computePoolInfo,
                                                      m_logicalDevice->get());

  const auto frameCount = m_swapChain->imageCount();
  m_frames.reserve(frameCount);
  for (uint32_t i = 0; i < frameCount; ++i) {
    m_frames.push_back(std::make_unique<VulkanFrame>(
        m_graphicsPool.get(), m_transferPool.get(), m_computePool.get(),
        m_logicalDevice->get()));
  }
  Util::println("Created vulkan renderer");
}

VulkanRenderer::~VulkanRenderer() {
  m_logicalDevice->waitIdle();
  Util::println("Destroyed vulkan renderer");
}