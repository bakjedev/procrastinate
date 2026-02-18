#include "vk_descriptor.hpp"

#include "util/print.hpp"

VulkanDescriptorPool::VulkanDescriptorPool(vk::Device device, const DescriptorPoolInfo &info) : device_(device)
{
  const vk::DescriptorPoolCreateInfo createInfo{.flags = info.flags,
                                                .maxSets = info.max_sets,
                                                .poolSizeCount = static_cast<uint32_t>(info.pool_sizes.size()),
                                                .pPoolSizes = info.pool_sizes.data()};

  descriptor_pool_ = device.createDescriptorPool(createInfo);
}

VulkanDescriptorPool::~VulkanDescriptorPool()
{
  if (descriptor_pool_)
  {
    device_.destroyDescriptorPool(descriptor_pool_);
  }
}

vk::DescriptorSet VulkanDescriptorPool::allocate(vk::DescriptorSetLayout layout) const
{
  const vk::DescriptorSetAllocateInfo allocInfo{
      .descriptorPool = descriptor_pool_, .descriptorSetCount = 1, .pSetLayouts = &layout};

  const auto sets = device_.allocateDescriptorSets(allocInfo);
  return sets.front();
}

std::vector<vk::DescriptorSet> VulkanDescriptorPool::allocate(const std::vector<vk::DescriptorSetLayout> &layouts) const
{
  const vk::DescriptorSetAllocateInfo allocInfo{.descriptorPool = descriptor_pool_,
                                                .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
                                                .pSetLayouts = layouts.data()};

  return device_.allocateDescriptorSets(allocInfo);
}

VulkanDescriptorSetLayout::VulkanDescriptorSetLayout(const vk::Device device,
                                                     const std::vector<vk::DescriptorSetLayoutBinding> &bindings,
                                                     const std::vector<vk::DescriptorBindingFlags> &bindingFlags,
                                                     const vk::DescriptorSetLayoutCreateFlags flags) : device_(device)
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

  layout_ = device.createDescriptorSetLayout(createInfo);
}

VulkanDescriptorSetLayout::~VulkanDescriptorSetLayout()
{
  if (layout_)
  {
    device_.destroyDescriptorSetLayout(layout_);
  }
}
