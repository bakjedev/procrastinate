#include "vk_instance.hpp"

#include <SDL3/SDL_vulkan.h>

#include "util/print.hpp"

#ifndef NDEBUG
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    [[maybe_unused]] vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
    [[maybe_unused]] vk::DebugUtilsMessageTypeFlagsEXT type,
    const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
    [[maybe_unused]] void* pUserData) {
  Util::println("Validation: {}", pCallbackData->pMessage);
  return VK_FALSE;
}
#endif

VulkanInstance::VulkanInstance() {
  std::vector<const char*> extensions;
  std::vector<const char*> layers;

  uint32_t sdlInstanceExtensionCount = 0;
  const auto* const sdlInstanceExtensions =
      SDL_Vulkan_GetInstanceExtensions(&sdlInstanceExtensionCount);
  if (sdlInstanceExtensions == nullptr) {
    throw std::runtime_error("Failed to get SDL Vulkan extensions");
  }

  extensions.reserve(sdlInstanceExtensionCount);
  for (uint32_t i = 0; i < sdlInstanceExtensionCount; i++) {
    extensions.push_back(sdlInstanceExtensions[i]);
  }

  constexpr vk::ApplicationInfo applicationInfo{
      .sType = vk::StructureType::eApplicationInfo,
      .pApplicationName = "game",
      .applicationVersion = 1,
      .pEngineName = "hell",
      .engineVersion = 1,
      .apiVersion = VK_API_VERSION_1_3};

  extensions.push_back(vk::EXTDebugUtilsExtensionName);
#ifndef NDEBUG
  layers.push_back("VK_LAYER_KHRONOS_validation");
#endif

  const vk::InstanceCreateInfo instanceCreateInfo{
      .sType = vk::StructureType::eInstanceCreateInfo,
      .pApplicationInfo = &applicationInfo,
      .enabledLayerCount = static_cast<uint32_t>(layers.size()),
      .ppEnabledLayerNames = layers.data(),
      .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
      .ppEnabledExtensionNames = extensions.data(),
  };

  m_instance = vk::createInstance(instanceCreateInfo);

  const vk::detail::DynamicLoader dynamicLoader;
  const auto vkGetInstanceProcAddr =
      dynamicLoader.getProcAddress<PFN_vkGetInstanceProcAddr>(
          "vkGetInstanceProcAddr");
  m_dynamicLoader.init(vkGetInstanceProcAddr);
  m_dynamicLoader.init(m_instance);
#ifndef NDEBUG

  constexpr vk::DebugUtilsMessengerCreateInfoEXT debugInfo{
      .messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                         // vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
                         vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
      .messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                     vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                     vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
      .pfnUserCallback = debugCallback};

  m_debugMessenger = m_instance.createDebugUtilsMessengerEXT(debugInfo, nullptr,
                                                             m_dynamicLoader);
#endif
}

VulkanInstance::~VulkanInstance() {
#ifndef NDEBUG
  m_instance.destroyDebugUtilsMessengerEXT(m_debugMessenger, nullptr,
                                           m_dynamicLoader);
#endif

  m_instance.destroy();
}
