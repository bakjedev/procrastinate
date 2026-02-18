#pragma once
#include <vulkan/vulkan.hpp>

struct SDL_Window;

class VulkanSurface
{
public:
  explicit VulkanSurface(SDL_Window *window, vk::Instance instance);
  VulkanSurface(const VulkanSurface &) = delete;
  VulkanSurface(VulkanSurface &&) = delete;
  VulkanSurface &operator=(const VulkanSurface &) = delete;
  VulkanSurface &operator=(VulkanSurface &&) = delete;
  ~VulkanSurface();

  [[nodiscard]] vk::SurfaceKHR get() const { return surface_; }

private:
  vk::SurfaceKHR surface_;

  vk::Instance instance_; // for deconstructing
};
