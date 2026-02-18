#include "render/vk_surface.hpp"

#include <SDL3/SDL_vulkan.h>

#include "util/print.hpp"

VulkanSurface::VulkanSurface(SDL_Window* window, const vk::Instance instance) : instance_(instance)
{
  VkSurfaceKHR c_surface = VK_NULL_HANDLE;

  if (!SDL_Vulkan_CreateSurface(window, instance_, nullptr, &c_surface))
  {
    throw std::runtime_error("Failed to create SDL Vulkan surface" + std::string(SDL_GetError()));
  }
  surface_ = c_surface;
}

VulkanSurface::~VulkanSurface() { SDL_Vulkan_DestroySurface(instance_, surface_, nullptr); }
