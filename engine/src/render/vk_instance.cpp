#include "vk_instance.hpp"

#include <SDL3/SDL_vulkan.h>

#include "util/print.hpp"

#ifndef NDEBUG
static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback([[maybe_unused]] vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
                                                    [[maybe_unused]] vk::DebugUtilsMessageTypeFlagsEXT type,
                                                    const vk::DebugUtilsMessengerCallbackDataEXT* p_callback_data,
                                                    [[maybe_unused]] void* p_user_data)
{
  util::println("Validation: {}", p_callback_data->pMessage);
  return VK_FALSE;
}
#endif

VulkanInstance::VulkanInstance()
{
  std::vector<const char*> extensions;
  std::vector<const char*> layers;

  uint32_t sdl_instance_extension_count = 0;
  const auto* const sdl_instance_extensions = SDL_Vulkan_GetInstanceExtensions(&sdl_instance_extension_count);
  if (sdl_instance_extensions == nullptr)
  {
    throw std::runtime_error("Failed to get SDL Vulkan extensions");
  }

  extensions.reserve(sdl_instance_extension_count);
  for (uint32_t i = 0; i < sdl_instance_extension_count; i++)
  {
    extensions.push_back(sdl_instance_extensions[i]);
  }

  constexpr vk::ApplicationInfo application_info{.sType = vk::StructureType::eApplicationInfo,
                                                 .pApplicationName = "game",
                                                 .applicationVersion = 1,
                                                 .pEngineName = "hell",
                                                 .engineVersion = 1,
                                                 .apiVersion = VK_API_VERSION_1_3};

  extensions.push_back(vk::EXTDebugUtilsExtensionName);
#ifndef NDEBUG
  layers.push_back("VK_LAYER_KHRONOS_validation");
#endif

  const vk::InstanceCreateInfo instance_create_info{
      .sType = vk::StructureType::eInstanceCreateInfo,
      .pApplicationInfo = &application_info,
      .enabledLayerCount = static_cast<uint32_t>(layers.size()),
      .ppEnabledLayerNames = layers.data(),
      .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
      .ppEnabledExtensionNames = extensions.data(),
  };

  instance_ = vk::createInstance(instance_create_info);

  const vk::detail::DynamicLoader dynamicLoader;
  const auto vk_get_instance_proc_addr =
      dynamicLoader.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
  dynamic_loader_.init(vk_get_instance_proc_addr);
  dynamic_loader_.init(instance_);
#ifndef NDEBUG

  constexpr vk::DebugUtilsMessengerCreateInfoEXT debugInfo{
      .messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                         // vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
                         vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
      .messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                     vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                     vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
      .pfnUserCallback = DebugCallback};

  debug_messenger_ = instance_.createDebugUtilsMessengerEXT(debugInfo, nullptr, dynamic_loader_);
#endif
}

VulkanInstance::~VulkanInstance()
{
#ifndef NDEBUG
  instance_.destroyDebugUtilsMessengerEXT(debug_messenger_, nullptr, dynamic_loader_);
#endif

  instance_.destroy();
}
