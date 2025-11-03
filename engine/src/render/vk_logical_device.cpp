#include "render/vk_logical_device.hpp"

#include <set>
#include <stdexcept>
#include <vector>

#include "render/vk_physical_device.hpp"
#include "util/util.hpp"

VulkanLogicalDevice::VulkanLogicalDevice(
    VkPhysicalDevice physicalDevice, const VkPhysicalDeviceFeatures& features,
    const QueueFamilyIndices& queueFamilyIndices,
    const VkPhysicalDeviceVulkan12Features& features12,
    const VkPhysicalDeviceVulkan13Features& features13) {
  const std::set uniqueQueueFamilies = {
      queueFamilyIndices.graphics.value(), queueFamilyIndices.compute.value(),
      queueFamilyIndices.transfer.value(), queueFamilyIndices.present.value()};

  constexpr float queuePriority = 1.0F;
  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

  for (const uint32_t queueFamily : uniqueQueueFamilies) {
    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = queueFamily;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;
    queueCreateInfos.push_back(queueCreateInfo);
  }

  std::vector deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

  // enable features that were queried
  VkPhysicalDeviceVulkan13Features enabled13Features{};
  enabled13Features.sType =
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
  enabled13Features.pNext = nullptr;
  enabled13Features.dynamicRendering = features13.dynamicRendering;
  enabled13Features.synchronization2 = features13.synchronization2;

  VkPhysicalDeviceVulkan12Features enabled12Features{};
  enabled12Features.sType =
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
  enabled12Features.pNext = &enabled13Features;
  enabled12Features.descriptorIndexing = features12.descriptorIndexing;
  enabled12Features.descriptorBindingPartiallyBound =
      features12.descriptorBindingPartiallyBound;
  enabled12Features.runtimeDescriptorArray = features12.runtimeDescriptorArray;
  enabled12Features.bufferDeviceAddress = features12.bufferDeviceAddress;

  const VkDeviceCreateInfo deviceCreateInfo{
      .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
      .pNext = &enabled12Features,
      .flags = 0,
      .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
      .pQueueCreateInfos = queueCreateInfos.data(),
      .enabledLayerCount = 0,
      .ppEnabledLayerNames = nullptr,
      .enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
      .ppEnabledExtensionNames = deviceExtensions.data(),
      .pEnabledFeatures = &features};

  VK_CHECK(vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr,
                          &m_logicalDevice));

  vkGetDeviceQueue(m_logicalDevice, queueFamilyIndices.graphics.value(), 0,
                   &m_graphicsQueue);
  vkGetDeviceQueue(m_logicalDevice, queueFamilyIndices.compute.value(), 0,
                   &m_computeQueue);
  vkGetDeviceQueue(m_logicalDevice, queueFamilyIndices.transfer.value(), 0,
                   &m_transferQueue);
  vkGetDeviceQueue(m_logicalDevice, queueFamilyIndices.present.value(), 0,
                   &m_presentQueue);

  if (m_graphicsQueue == VK_NULL_HANDLE || m_computeQueue == VK_NULL_HANDLE ||
      m_transferQueue == VK_NULL_HANDLE || m_presentQueue == VK_NULL_HANDLE) {
    throw std::runtime_error(
        "Unable to get (at least) one of the Vulkan logical device queues");
  }

  Util::println("Created vulkan logical device with queues");
}

VulkanLogicalDevice::~VulkanLogicalDevice() {
  vkDestroyDevice(m_logicalDevice, nullptr);
  Util::println("Destroyed vulkan logical device");
}

void VulkanLogicalDevice::waitIdle() {
  VK_CHECK(vkDeviceWaitIdle(m_logicalDevice));
}