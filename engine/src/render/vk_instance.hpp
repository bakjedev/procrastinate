#pragma once
#include <vulkan/vulkan.h>

class VulkanInstance
{
public:
	explicit VulkanInstance();
	~VulkanInstance();
	[[nodiscard]] VkInstance get() const { return m_instance; }

private:
	VkInstance m_instance{};
};