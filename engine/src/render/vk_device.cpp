#include "render/vk_device.hpp"

#include <set>

#include "util/print.hpp"

VulkanDevice::VulkanDevice(const vk::Instance instance,
                           const vk::SurfaceKHR surface) {
  pickPhysicalDevice(instance, surface);
  createDevice();
  getQueues();
}

VulkanDevice::~VulkanDevice() { m_device.destroy(); }

void VulkanDevice::waitIdle() const { m_device.waitIdle(); }

void VulkanDevice::pickPhysicalDevice(vk::Instance instance,
                                      vk::SurfaceKHR surface) {
  const auto devices = instance.enumeratePhysicalDevices();

  if (devices.empty()) {
    throw std::runtime_error("Failed to find GPUs with Vulkan support");
  }

  for (const auto& device : devices) {
    vk::PhysicalDeviceProperties properties{};
    vk::PhysicalDeviceVulkan13Features features13{};
    vk::PhysicalDeviceVulkan12Features features12{.pNext = &features13};
    vk::PhysicalDeviceVulkan11Features features11{.pNext = &features12};
    vk::PhysicalDeviceFeatures2 features{.pNext = &features11};
    device.getProperties(&properties);
    device.getFeatures2(&features);
    if (isDeviceSuitable(properties)) {
      m_physicalDevice = device;
      m_properties = properties;
      m_availableFeatures = features;
      m_availableFeatures12 = features12;
      m_availableFeatures11 = features11;
      m_availableFeatures13 = features13;

      if (!findQueueFamilies(surface)) {
        throw std::runtime_error("Failed to find Vulkan queue families");
      }

      Util::println("Picked physical device: {}", properties.deviceName.data());

      break;
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
    const vk::DeviceQueueCreateInfo queueCreateInfo{
        .queueFamilyIndex = queueFamily,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority};
    queueCreateInfos.push_back(queueCreateInfo);
  }

  const std::vector deviceExtensions = {vk::KHRSwapchainExtensionName};

  m_enabledFeatures = {.pNext = m_enabledFeatures11};
  m_enabledFeatures11 = {.pNext = m_enabledFeatures12};
  m_enabledFeatures12 = {.pNext = m_enabledFeatures13};
  m_enabledFeatures13 = {};

  m_enabledFeatures.features.multiDrawIndirect =
      m_availableFeatures.features.multiDrawIndirect;

  m_enabledFeatures11.shaderDrawParameters =
      m_availableFeatures11.shaderDrawParameters;

  m_enabledFeatures13.synchronization2 = m_availableFeatures13.synchronization2;
  m_enabledFeatures13.dynamicRendering = m_availableFeatures13.dynamicRendering;

  m_enabledFeatures12.descriptorIndexing =
      m_availableFeatures12.descriptorIndexing;

  m_enabledFeatures12.descriptorBindingUniformBufferUpdateAfterBind =
      m_availableFeatures12.descriptorBindingUniformBufferUpdateAfterBind;
  m_enabledFeatures12.descriptorBindingSampledImageUpdateAfterBind =
      m_availableFeatures12.descriptorBindingSampledImageUpdateAfterBind;
  m_enabledFeatures12.descriptorBindingStorageImageUpdateAfterBind =
      m_availableFeatures12.descriptorBindingStorageImageUpdateAfterBind;
  m_enabledFeatures12.descriptorBindingStorageBufferUpdateAfterBind =
      m_availableFeatures12.descriptorBindingStorageBufferUpdateAfterBind;
  m_enabledFeatures12.descriptorBindingPartiallyBound =
      m_availableFeatures12.descriptorBindingPartiallyBound;
  m_enabledFeatures12.descriptorBindingVariableDescriptorCount =
      m_availableFeatures12.descriptorBindingVariableDescriptorCount;

  m_enabledFeatures12.runtimeDescriptorArray =
      m_availableFeatures12.runtimeDescriptorArray;
  m_enabledFeatures12.bufferDeviceAddress =
      m_availableFeatures12.bufferDeviceAddress;

  vk::DeviceCreateInfo deviceCreateInfo;
  deviceCreateInfo.pNext = &m_enabledFeatures;
  deviceCreateInfo.queueCreateInfoCount =
      static_cast<uint32_t>(queueCreateInfos.size());
  deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
  deviceCreateInfo.enabledExtensionCount =
      static_cast<uint32_t>(deviceExtensions.size());
  deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
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

bool VulkanDevice::isDeviceSuitable(const vk::PhysicalDeviceProperties&) {
  return true;  // there should be requirements
}

bool VulkanDevice::findQueueFamilies(const vk::SurfaceKHR surface) {
  const auto queueFamilies = m_physicalDevice.getQueueFamilyProperties();

  for (size_t idx = 0; idx < queueFamilies.size(); idx++) {
    const auto& queueFamily = queueFamilies.at(idx);

    // Dedicated compute queue
    if ((queueFamily.queueFlags & vk::QueueFlagBits::eCompute) &&
        !(queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)) {
      m_queueFamilyIndices.compute = idx;
    }

    // Dedicated transfer queue
    if (queueFamily.queueFlags & vk::QueueFlagBits::eTransfer &&
        !(queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) &&
        !(queueFamily.queueFlags & vk::QueueFlagBits::eCompute)) {
      m_queueFamilyIndices.transfer = idx;
    }
  }

  // Graphics and present. Also compute and transfer if it couldn't find
  // dedicated queues for them
  for (size_t idx = 0; idx < queueFamilies.size(); idx++) {
    const auto& queueFamily = queueFamilies.at(idx);

    if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
      m_queueFamilyIndices.graphics = idx;
    }

    const auto presentSupport =
        m_physicalDevice.getSurfaceSupportKHR(idx, surface);
    if (presentSupport != 0U) {
      m_queueFamilyIndices.present = idx;
    }

    if (!m_queueFamilyIndices.compute.has_value() &&
        (queueFamily.queueFlags & vk::QueueFlagBits::eCompute)) {
      m_queueFamilyIndices.compute = idx;
    }

    if (!m_queueFamilyIndices.transfer.has_value() &&
        (queueFamily.queueFlags & vk::QueueFlagBits::eTransfer)) {
      m_queueFamilyIndices.transfer = idx;
    }
    if (m_queueFamilyIndices.isComplete()) {
      return true;
    }
  }

  return m_queueFamilyIndices.isComplete();
}
