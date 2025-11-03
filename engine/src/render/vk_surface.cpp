#include "render/vk_surface.hpp"

#include <SDL3/SDL_vulkan.h>

#include "util/util.hpp"

VulkanSurface::VulkanSurface(SDL_Window* window, VkInstance instance)
    : m_instance(instance) {
  if (!SDL_Vulkan_CreateSurface(window, m_instance, nullptr, &m_surface)) {
    throw std::runtime_error("Failed to create SDL Vulkan surface" +
                             std::string(SDL_GetError()));
  }
  Util::println("Created vulkan surface");
}

VulkanSurface::~VulkanSurface() {
  SDL_Vulkan_DestroySurface(m_instance, m_surface, nullptr);
  Util::println("Destroyed vulkan surface");
}
