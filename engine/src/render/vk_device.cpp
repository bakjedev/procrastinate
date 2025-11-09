#include "render/vk_device.hpp"

#include <set>

#include "util/print.hpp"
#include "vk_device.hpp"

VulkanDevice::VulkanDevice(vk::Instance instance, vk::SurfaceKHR surface) {
  pickPhysicalDevice(instance, surface);
  createDevice();
  getQueues();
  Util::println(
      "Picked physical device, created vulkan logical device, and got queues");
}

VulkanDevice::~VulkanDevice() { m_device.destroy(); }

void VulkanDevice::waitIdle() { m_device.waitIdle(); }

void VulkanDevice::pickPhysicalDevice(vk::Instance instance,
                                      vk::SurfaceKHR surface) {
  auto devices = instance.enumeratePhysicalDevices();

  if (devices.empty()) {
    throw std::runtime_error("Failed to find GPUs with Vulkan support");
  }

  for (const auto& device : devices) {
    auto properties = device.getProperties();
    auto features = device.getFeatures();
    if (isDeviceSuitable(properties, features)) {
      m_physicalDevice = device;
      m_properties = properties;
      m_features = features;

      m_features13.pNext = nullptr;
      m_features12.pNext = &m_features13;

      vk::PhysicalDeviceFeatures2 features2;
      features2.pNext = &m_features12;

      m_physicalDevice.getFeatures2(&features2);

      if (!findQueueFamilies(surface)) {
        throw std::runtime_error("Failed to find Vulkan queue families");
      }

      Util::println("Picked vulkan physical device ({})",
                    properties.deviceName.data());
    }
  }
}

void VulkanDevice::createDevice() {
  const std::set uniqueQueueFamilies = {m_queueFamilyIndices.graphics.value(),
                                        m_queueFamilyIndices.compute.value(),
                                        m_queueFamilyIndices.transfer.value(),
                                        m_queueFamilyIndices.present.value()};

  constexpr float queuePriority = 1.0F;
  std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;

  for (const uint32_t queueFamily : uniqueQueueFamilies) {
    vk::DeviceQueueCreateInfo queueCreateInfo{
        .queueFamilyIndex = queueFamily,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority};
    queueCreateInfos.push_back(queueCreateInfo);
  }

  std::vector deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

  vk::PhysicalDeviceVulkan13Features enabled13Features{
      .pNext = nullptr,
      .synchronization2 = m_features13.synchronization2,
      .dynamicRendering = m_features13.dynamicRendering};

  vk::PhysicalDeviceVulkan12Features enabled12Features{
      .pNext = &enabled13Features,
      .descriptorIndexing = m_features12.descriptorIndexing,
      .descriptorBindingUniformBufferUpdateAfterBind =
          m_features12.descriptorBindingUniformBufferUpdateAfterBind,
      .descriptorBindingSampledImageUpdateAfterBind =
          m_features12.descriptorBindingSampledImageUpdateAfterBind,
      .descriptorBindingStorageImageUpdateAfterBind =
          m_features12.descriptorBindingStorageImageUpdateAfterBind,
      .descriptorBindingStorageBufferUpdateAfterBind =
          m_features12.descriptorBindingStorageBufferUpdateAfterBind,
      .descriptorBindingPartiallyBound =
          m_features12.descriptorBindingPartiallyBound,
      .descriptorBindingVariableDescriptorCount =
          m_features12.descriptorBindingVariableDescriptorCount,
      .runtimeDescriptorArray = m_features12.runtimeDescriptorArray,
      .bufferDeviceAddress = m_features12.bufferDeviceAddress};

  const vk::DeviceCreateInfo deviceCreateInfo{
      .pNext = &enabled12Features,
      .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
      .pQueueCreateInfos = queueCreateInfos.data(),
      .enabledLayerCount = 0,
      .ppEnabledLayerNames = nullptr,
      .enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
      .ppEnabledExtensionNames = deviceExtensions.data(),
      .pEnabledFeatures = &m_features};

  m_device = m_physicalDevice.createDevice(deviceCreateInfo);
}

void VulkanDevice::getQueues() {
  m_graphicsQueue = m_device.getQueue(m_queueFamilyIndices.graphics.value(), 0);
  m_computeQueue = m_device.getQueue(m_queueFamilyIndices.compute.value(), 0);
  m_transferQueue = m_device.getQueue(m_queueFamilyIndices.transfer.value(), 0);
  m_presentQueue = m_device.getQueue(m_queueFamilyIndices.present.value(), 0);

  if (!m_graphicsQueue || !m_computeQueue || !m_transferQueue ||
      !m_presentQueue) {
    throw std::runtime_error(
        "Unable to get at least one of the Vulkan logical device queues");
  }
}

bool VulkanDevice::isDeviceSuitable(
    const vk::PhysicalDeviceProperties& properties,
    const vk::PhysicalDeviceFeatures& features) {
  return properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu &&
         (features.geometryShader != 0U);
}

bool VulkanDevice::findQueueFamilies(vk::SurfaceKHR surface) {
  auto queueFamilies = m_physicalDevice.getQueueFamilyProperties();

  int index = 0;
  for (const auto& queue_family : queueFamilies) {
    if (queue_family.queueFlags & vk::QueueFlagBits::eGraphics) {
      m_queueFamilyIndices.graphics = index;
    }

    auto presentSupport = m_physicalDevice.getSurfaceSupportKHR(index, surface);
    if (presentSupport != 0U) {
      m_queueFamilyIndices.present = index;
    }

    if (queueFamilies[index].queueFlags & vk::QueueFlagBits::eCompute) {
      m_queueFamilyIndices.compute = index;
    }

    if ((queue_family.queueFlags & vk::QueueFlagBits::eTransfer) &&
        !(queue_family.queueFlags & vk::QueueFlagBits::eGraphics)) {
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
