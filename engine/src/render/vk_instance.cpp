#include <SDL3/SDL_vulkan.h>

#include <render/vk_instance.hpp>
#include <vector>

#include "util/vk_check.hpp"

VulkanInstance::VulkanInstance() {
  uint32_t sdlInstanceExtensionCount = 0;
  const auto *const sdlInstanceExtensions =
      SDL_Vulkan_GetInstanceExtensions(&sdlInstanceExtensionCount);
  if (sdlInstanceExtensions == nullptr) {
    throw std::runtime_error("Failed to get SDL Vulkan extensions");
  }

  std::vector extensions(sdlInstanceExtensions,
                         sdlInstanceExtensions + sdlInstanceExtensionCount);

  const VkApplicationInfo applicationInfo{
      .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
      .pNext = nullptr,
      .pApplicationName = "game",
      .applicationVersion = 1,
      .pEngineName = "hell",
      .engineVersion = 1,
      .apiVersion = VK_API_VERSION_1_3};

  const VkInstanceCreateInfo instanceCreateInfo{
      .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .pApplicationInfo = &applicationInfo,
      .enabledLayerCount = 0,
      .ppEnabledLayerNames = nullptr,
      .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
      .ppEnabledExtensionNames = extensions.data(),
  };

  VK_CHECK(vkCreateInstance(&instanceCreateInfo, nullptr, &m_instance));
  Util::println("Created vulkan instance");
}

VulkanInstance::~VulkanInstance() {
  vkDestroyInstance(m_instance, nullptr);
  Util::println("Destroyed vulkan instance");
}
