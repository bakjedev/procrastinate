#include "vk_descriptor.hpp"

#include "util/print.hpp"

VulkanDescriptorPool::VulkanDescriptorPool(vk::Device device, const DescriptorPoolInfo &info) : m_device(device)
{
  const vk::DescriptorPoolCreateInfo createInfo{.flags = info.flags,
                                                .maxSets = info.maxSets,
                                                .poolSizeCount = static_cast<uint32_t>(info.poolSizes.size()),
                                                .pPoolSizes = info.poolSizes.data()};

  m_descriptorPool = device.createDescriptorPool(createInfo);
}

VulkanDescriptorPool::~VulkanDescriptorPool()
{
  if (m_descriptorPool)
  {
    m_device.destroyDescriptorPool(m_descriptorPool);
  }
}

vk::DescriptorSet VulkanDescriptorPool::allocate(vk::DescriptorSetLayout layout) const
{
  const vk::DescriptorSetAllocateInfo allocInfo{
      .descriptorPool = m_descriptorPool, .descriptorSetCount = 1, .pSetLayouts = &layout};

  const auto sets = m_device.allocateDescriptorSets(allocInfo);
  return sets.front();
}

std::vector<vk::DescriptorSet> VulkanDescriptorPool::allocate(const std::vector<vk::DescriptorSetLayout> &layouts) const
{
  const vk::DescriptorSetAllocateInfo allocInfo{.descriptorPool = m_descriptorPool,
                                                .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
                                                .pSetLayouts = layouts.data()};

  return m_device.allocateDescriptorSets(allocInfo);
}

VulkanDescriptorSetLayout::VulkanDescriptorSetLayout(const vk::Device device,
                                                     const std::vector<vk::DescriptorSetLayoutBinding> &bindings,
                                                     const std::vector<vk::DescriptorBindingFlags> &bindingFlags,
                                                     const vk::DescriptorSetLayoutCreateFlags flags) : m_device(device)
{
  vk::DescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsInfo;
  if (!bindingFlags.empty())
  {
    bindingFlagsInfo.bindingCount = static_cast<uint32_t>(bindingFlags.size());
    bindingFlagsInfo.pBindingFlags = bindingFlags.data();
  }

  const vk::DescriptorSetLayoutCreateInfo createInfo{.pNext = !bindingFlags.empty() ? &bindingFlagsInfo : nullptr,
                                                     .flags = flags,
                                                     .bindingCount = static_cast<uint32_t>(bindings.size()),
                                                     .pBindings = bindings.data()};

  m_layout = device.createDescriptorSetLayout(createInfo);
}

VulkanDescriptorSetLayout::~VulkanDescriptorSetLayout()
{
  if (m_layout)
  {
    m_device.destroyDescriptorSetLayout(m_layout);
  }
}
