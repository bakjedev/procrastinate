#include "vk_instance.hpp"

#include <SDL3/SDL_vulkan.h>

#include "util/print.hpp"

VulkanInstance::VulkanInstance() {
  uint32_t sdlInstanceExtensionCount = 0;
  const auto *const sdlInstanceExtensions =
      SDL_Vulkan_GetInstanceExtensions(&sdlInstanceExtensionCount);
  if (sdlInstanceExtensions == nullptr) {
    throw std::runtime_error("Failed to get SDL Vulkan extensions");
  }

  const vk::ApplicationInfo applicationInfo{
      .sType = vk::StructureType::eApplicationInfo,
      .pApplicationName = "game",
      .applicationVersion = 1,
      .pEngineName = "hell",
      .engineVersion = 1,
      .apiVersion = VK_API_VERSION_1_3};

  const vk::InstanceCreateInfo instanceCreateInfo{
      .sType = vk::StructureType::eInstanceCreateInfo,
      .pApplicationInfo = &applicationInfo,
      .enabledExtensionCount = sdlInstanceExtensionCount,
      .ppEnabledExtensionNames = sdlInstanceExtensions,
  };

  m_instance = vk::createInstance(instanceCreateInfo);
  Util::println("Created vulkan instance");
}

VulkanInstance::~VulkanInstance() {
  vkDestroyInstance(m_instance, nullptr);
  Util::println("Destroyed vulkan instance");
}
