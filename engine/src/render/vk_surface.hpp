#pragma once
#include <vulkan/vulkan.h>

struct SDL_Window;

class VulkanSurface {
 public:
  explicit VulkanSurface(SDL_Window *window, VkInstance instance);
  VulkanSurface(const VulkanSurface &) = delete;
  VulkanSurface(VulkanSurface &&) = delete;
  VulkanSurface &operator=(const VulkanSurface &) = delete;
  VulkanSurface &operator=(VulkanSurface &&) = delete;
  ~VulkanSurface();

  [[nodiscard]] VkSurfaceKHR get() const { return m_surface; }

 private:
  VkSurfaceKHR m_surface{};

  VkInstance m_instance{};  // for deconstructing
};