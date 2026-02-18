#include "render/vk_surface.hpp"

#include <SDL3/SDL_vulkan.h>

#include "util/print.hpp"

VulkanSurface::VulkanSurface(SDL_Window* window, const vk::Instance instance) : m_instance(instance)
{
  VkSurfaceKHR cSurface = VK_NULL_HANDLE;

  if (!SDL_Vulkan_CreateSurface(window, m_instance, nullptr, &cSurface))
  {
    throw std::runtime_error("Failed to create SDL Vulkan surface" + std::string(SDL_GetError()));
  }
  m_surface = cSurface;
}

VulkanSurface::~VulkanSurface() { SDL_Vulkan_DestroySurface(m_instance, m_surface, nullptr); }
