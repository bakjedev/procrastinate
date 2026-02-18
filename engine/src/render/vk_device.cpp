#include "render/vk_device.hpp"

#include <set>

#include "util/print.hpp"

VulkanDevice::VulkanDevice(const vk::Instance instance, const vk::SurfaceKHR surface)
{
  PickPhysicalDevice(instance, surface);
  CreateDevice();
  GetQueues();
}

VulkanDevice::~VulkanDevice() { device_.destroy(); }

void VulkanDevice::WaitIdle() const { device_.waitIdle(); }

void VulkanDevice::PickPhysicalDevice(vk::Instance instance, vk::SurfaceKHR surface)
{
  const auto devices = instance.enumeratePhysicalDevices();

  if (devices.empty())
  {
    throw std::runtime_error("Failed to find GPUs with Vulkan support");
  }

  for (const auto& device: devices)
  {
    vk::PhysicalDeviceProperties properties{};
    vk::PhysicalDeviceVulkan13Features features13{};
    vk::PhysicalDeviceVulkan12Features features12{.pNext = &features13};
    vk::PhysicalDeviceVulkan11Features features11{.pNext = &features12};
    vk::PhysicalDeviceFeatures2 features{.pNext = &features11};
    device.getProperties(&properties);
    device.getFeatures2(&features);
    if (IsDeviceSuitable(properties))
    {
      physical_device_ = device;
      properties_ = properties;
      available_features_ = features;
      available_features12_ = features12;
      available_features11_ = features11;
      available_features13_ = features13;

      if (!FindQueueFamilies(surface))
      {
        throw std::runtime_error("Failed to find Vulkan queue families");
      }

      util::println("Picked physical device: {}", properties.deviceName.data());

      break;
    }
  }
  if (physical_device_ == nullptr)
  {
    throw std::runtime_error("Failed to pick physical device");
  }
}

void VulkanDevice::CreateDevice()
{
  const std::set unique_queue_families = {queue_family_indices_.graphics.value(), queue_family_indices_.compute.value(),
                                          queue_family_indices_.transfer.value(),
                                          queue_family_indices_.present.value()};

  constexpr float queue_priority = 1.0F;
  std::vector<vk::DeviceQueueCreateInfo> queue_create_infos;

  for (const uint32_t queue_family: unique_queue_families)
  {
    const vk::DeviceQueueCreateInfo queue_create_info{
        .queueFamilyIndex = queue_family, .queueCount = 1, .pQueuePriorities = &queue_priority};
    queue_create_infos.push_back(queue_create_info);
  }

  const std::vector device_extensions = {vk::KHRSwapchainExtensionName};

  enabled_features_ = {.pNext = enabled_features11_};
  enabled_features11_ = {.pNext = enabled_features12_};
  enabled_features12_ = {.pNext = enabled_features13_};
  enabled_features13_ = {};

  enabled_features_.features.multiDrawIndirect = available_features_.features.multiDrawIndirect;

  enabled_features11_.shaderDrawParameters = available_features11_.shaderDrawParameters;

  enabled_features13_.synchronization2 = available_features13_.synchronization2;
  enabled_features13_.dynamicRendering = available_features13_.dynamicRendering;

  enabled_features12_.descriptorIndexing = available_features12_.descriptorIndexing;

  enabled_features12_.descriptorBindingUniformBufferUpdateAfterBind =
      available_features12_.descriptorBindingUniformBufferUpdateAfterBind;
  enabled_features12_.descriptorBindingSampledImageUpdateAfterBind =
      available_features12_.descriptorBindingSampledImageUpdateAfterBind;
  enabled_features12_.descriptorBindingStorageImageUpdateAfterBind =
      available_features12_.descriptorBindingStorageImageUpdateAfterBind;
  enabled_features12_.descriptorBindingStorageBufferUpdateAfterBind =
      available_features12_.descriptorBindingStorageBufferUpdateAfterBind;
  enabled_features12_.descriptorBindingPartiallyBound = available_features12_.descriptorBindingPartiallyBound;
  enabled_features12_.descriptorBindingVariableDescriptorCount =
      available_features12_.descriptorBindingVariableDescriptorCount;

  enabled_features12_.runtimeDescriptorArray = available_features12_.runtimeDescriptorArray;
  enabled_features12_.bufferDeviceAddress = available_features12_.bufferDeviceAddress;

  enabled_features12_.drawIndirectCount = available_features12_.drawIndirectCount;
  vk::DeviceCreateInfo device_create_info{};
  device_create_info.pNext = &enabled_features_;
  device_create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
  device_create_info.pQueueCreateInfos = queue_create_infos.data();
  device_create_info.enabledExtensionCount = static_cast<uint32_t>(device_extensions.size());
  device_create_info.ppEnabledExtensionNames = device_extensions.data();
  device_ = physical_device_.createDevice(device_create_info);
}

void VulkanDevice::GetQueues()
{
  graphics_queue_ = device_.getQueue(queue_family_indices_.graphics.value(), 0);
  compute_queue_ = device_.getQueue(queue_family_indices_.compute.value(), 0);
  transfer_queue_ = device_.getQueue(queue_family_indices_.transfer.value(), 0);
  present_queue_ = device_.getQueue(queue_family_indices_.present.value(), 0);

  if (!graphics_queue_ || !compute_queue_ || !transfer_queue_ || !present_queue_)
  {
    throw std::runtime_error("Unable to get at least one of the Vulkan logical device queues");
  }
}

bool VulkanDevice::IsDeviceSuitable(const vk::PhysicalDeviceProperties& properties)
{
  return properties.limits.maxPushConstantsSize >= 96; // Frustum is 96 bytes
}

bool VulkanDevice::FindQueueFamilies(const vk::SurfaceKHR surface)
{
  const auto queue_families = physical_device_.getQueueFamilyProperties();

  for (size_t idx = 0; idx < queue_families.size(); idx++)
  {
    const auto& queue_family = queue_families.at(idx);

    // Dedicated compute queue
    if ((queue_family.queueFlags & vk::QueueFlagBits::eCompute) &&
        !(queue_family.queueFlags & vk::QueueFlagBits::eGraphics))
    {
      queue_family_indices_.compute = idx;
    }

    // Dedicated transfer queue
    if (queue_family.queueFlags & vk::QueueFlagBits::eTransfer &&
        !(queue_family.queueFlags & vk::QueueFlagBits::eGraphics) &&
        !(queue_family.queueFlags & vk::QueueFlagBits::eCompute))
    {
      queue_family_indices_.transfer = idx;
    }
  }

  // Graphics and present. Also compute and transfer if it couldn't find
  // dedicated queues for them
  for (size_t idx = 0; idx < queue_families.size(); idx++)
  {
    const auto& queue_family = queue_families.at(idx);

    if (queue_family.queueFlags & vk::QueueFlagBits::eGraphics)
    {
      queue_family_indices_.graphics = idx;
    }

    const auto present_support = physical_device_.getSurfaceSupportKHR(idx, surface);
    if (present_support != 0U)
    {
      queue_family_indices_.present = idx;
    }

    if (!queue_family_indices_.compute.has_value() && (queue_family.queueFlags & vk::QueueFlagBits::eCompute))
    {
      queue_family_indices_.compute = idx;
    }

    if (!queue_family_indices_.transfer.has_value() && (queue_family.queueFlags & vk::QueueFlagBits::eTransfer))
    {
      queue_family_indices_.transfer = idx;
    }
    if (queue_family_indices_.IsComplete())
    {
      return true;
    }
  }

  return queue_family_indices_.IsComplete();
}
