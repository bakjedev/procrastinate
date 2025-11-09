#pragma once
#include <vulkan/vulkan.hpp>

struct SDL_Window;

class VulkanSurface {
 public:
  explicit VulkanSurface(SDL_Window *window, vk::Instance instance);
  VulkanSurface(const VulkanSurface &) = delete;
  VulkanSurface(VulkanSurface &&) = delete;
  VulkanSurface &operator=(const VulkanSurface &) = delete;
  VulkanSurface &operator=(VulkanSurface &&) = delete;
  ~VulkanSurface();

  [[nodiscard]] vk::SurfaceKHR get() const { return m_surface; }

 private:
  vk::SurfaceKHR m_surface;

  vk::Instance m_instance;  // for deconstructing
};