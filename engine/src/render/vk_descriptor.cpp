#include "vk_descriptor.hpp"

#include "util/print.hpp"

VulkanDescriptorPool::VulkanDescriptorPool(vk::Device device,
                                           const DescriptorPoolInfo &info)
    : m_device(device) {
  vk::DescriptorPoolCreateInfo createInfo{
      .flags = info.flags,
      .maxSets = info.maxSets,
      .poolSizeCount = static_cast<uint32_t>(info.poolSizes.size()),
      .pPoolSizes = info.poolSizes.data()};

  m_descriptorPool = device.createDescriptorPool(createInfo);
  Util::println("Created descriptor pool");
}

VulkanDescriptorPool::~VulkanDescriptorPool() {
  if (m_descriptorPool) {
    m_device.destroyDescriptorPool(m_descriptorPool);
  }
  Util::println("Destroyed descriptor pool");
}

vk::DescriptorSet VulkanDescriptorPool::allocate(
    vk::DescriptorSetLayout layout) {
  vk::DescriptorSetAllocateInfo allocInfo{.descriptorPool = m_descriptorPool,
                                          .descriptorSetCount = 1,
                                          .pSetLayouts = &layout};

  auto sets = m_device.allocateDescriptorSets(allocInfo);
  return sets[0];
}

std::vector<vk::DescriptorSet> VulkanDescriptorPool::allocate(
    const std::vector<vk::DescriptorSetLayout> &layouts) {
  vk::DescriptorSetAllocateInfo allocInfo{
      .descriptorPool = m_descriptorPool,
      .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
      .pSetLayouts = layouts.data()};

  return m_device.allocateDescriptorSets(allocInfo);
}

VulkanDescriptorSetLayout::VulkanDescriptorSetLayout(
    vk::Device device,
    const std::vector<vk::DescriptorSetLayoutBinding> &bindings,
    const std::vector<vk::DescriptorBindingFlags> &bindingFlags,
    vk::DescriptorSetLayoutCreateFlags flags)
    : m_device(device) {
  vk::DescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsInfo;
  if (!bindingFlags.empty()) {
    bindingFlagsInfo.bindingCount = static_cast<uint32_t>(bindingFlags.size());
    bindingFlagsInfo.pBindingFlags = bindingFlags.data();
  }

  vk::DescriptorSetLayoutCreateInfo createInfo{
      .pNext = !bindingFlags.empty() ? &bindingFlagsInfo : nullptr,
      .flags = flags,
      .bindingCount = static_cast<uint32_t>(bindings.size()),
      .pBindings = bindings.data()};

  m_layout = device.createDescriptorSetLayout(createInfo);
}

VulkanDescriptorSetLayout::~VulkanDescriptorSetLayout() {
  if (m_layout) {
    m_device.destroyDescriptorSetLayout(m_layout);
  }
}