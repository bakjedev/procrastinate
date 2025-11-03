#pragma once
#include <memory>

#include "render/vk_allocator.hpp"
#include "render/vk_command_pool.hpp"
#include "render/vk_frame.hpp"
#include "render/vk_instance.hpp"
#include "render/vk_logical_device.hpp"
#include "render/vk_physical_device.hpp"
#include "render/vk_surface.hpp"
#include "render/vk_swap_chain.hpp"

struct SDL_Window;

class VulkanRenderer {
 public:
  VulkanRenderer(SDL_Window* window);
  ~VulkanRenderer();

 private:
  std::unique_ptr<VulkanInstance> m_instance;
  std::unique_ptr<VulkanSurface> m_surface;
  std::unique_ptr<VulkanPhysicalDevice> m_physicalDevice;
  std::unique_ptr<VulkanLogicalDevice> m_logicalDevice;
  std::unique_ptr<VulkanAllocator> m_allocator;
  std::unique_ptr<VulkanSwapChain> m_swapChain;
  std::unique_ptr<VulkanCommandPool> m_graphicsPool;
  std::unique_ptr<VulkanCommandPool> m_transferPool;
  std::unique_ptr<VulkanCommandPool> m_computePool;

  std::vector<std::unique_ptr<VulkanFrame>> m_frames;
  uint32_t m_frameCount;
};