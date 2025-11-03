#pragma once
#include <vulkan/vulkan.h>

struct SDL_Window;

class VulkanSurface
{
public:
	explicit VulkanSurface(SDL_Window* window, VkInstance instance);
	~VulkanSurface();

	[[nodiscard]] VkSurfaceKHR get() const { return m_surface; }

private:
	VkSurfaceKHR m_surface{};

	VkInstance m_instance{}; // for deconstructing
};