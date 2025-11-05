#include "render/vk_physical_device.hpp"

#include <stdexcept>
#include <vector>

#include "util/vk_check.hpp"

VulkanPhysicalDevice::VulkanPhysicalDevice(VkInstance instance,
                                           VkSurfaceKHR surface) {
  uint32_t deviceCount = 0;
  VK_CHECK(vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr));

  if (deviceCount == 0) {
    throw std::runtime_error("Failed to find GPUs with Vulkan support");
  }

  std::vector<VkPhysicalDevice> devices(deviceCount);
  VK_CHECK(vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data()));

  for (const auto& device : devices) {
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(device, &properties);
    VkPhysicalDeviceFeatures features;
    vkGetPhysicalDeviceFeatures(device, &features);

    if (isDeviceSuitable(properties, features)) {
      m_physicalDevice = device;
      m_properties = properties;
      m_features = features;

      // query gpu for features
      m_features13.sType =
          VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
      m_features13.pNext = nullptr;

      m_features12.sType =
          VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
      m_features12.pNext = &m_features13;

      VkPhysicalDeviceFeatures2 features2{};
      features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
      features2.pNext = &m_features12;

      vkGetPhysicalDeviceFeatures2(m_physicalDevice, &features2);

      if (!findQueueFamilies(surface)) {
        throw std::runtime_error("Failed to find Vulkan queue families");
      }

      Util::println("Picked vulkan physical device ({})",
                    properties.deviceName);
      return;
    }
  }
  throw std::runtime_error("Failed to find a suitable GPU");
}

VulkanPhysicalDevice::~VulkanPhysicalDevice() {
  Util::println("Destroyed physical device");
}

bool VulkanPhysicalDevice::isDeviceSuitable(
    const VkPhysicalDeviceProperties& properties,
    const VkPhysicalDeviceFeatures& features) {
  return properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
         (features.geometryShader != 0U);
}

bool VulkanPhysicalDevice::findQueueFamilies(VkSurfaceKHR surface) {
  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyCount,
                                           nullptr);

  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyCount,
                                           queueFamilies.data());

  int index = 0;
  for (const auto& queue_family : queueFamilies) {
    if ((queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0U) {
      m_queueFamilyIndices.graphics = index;
    }

    VkBool32 presentSupport = 0U;
    VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(m_physicalDevice, index,
                                                  surface, &presentSupport));
    if (presentSupport != 0U) {
      m_queueFamilyIndices.present = index;
    }

    if ((queueFamilies[index].queueFlags & VK_QUEUE_COMPUTE_BIT) != 0U) {
      m_queueFamilyIndices.compute = index;
    }

    if (((queue_family.queueFlags & VK_QUEUE_TRANSFER_BIT) != 0U) &&
        ((queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0U)) {
      m_queueFamilyIndices.transfer = index;
    }

    if (m_queueFamilyIndices.isComplete()) {
      return true;
    }

    index++;
  }

  // fallback for transfer queue.
  if (!m_queueFamilyIndices.transfer.has_value() &&
      m_queueFamilyIndices.graphics.has_value()) {
    m_queueFamilyIndices.transfer = m_queueFamilyIndices.graphics;
  }

  return m_queueFamilyIndices.isComplete();
}
