#include "vk_descriptor.hpp"

#include "util/print.hpp"

VulkanDescriptorPool::VulkanDescriptorPool(const vk::Device device, const DescriptorPoolInfo &info) : device_(device)
{
  const vk::DescriptorPoolCreateInfo create_info{.flags = info.flags,
                                                 .maxSets = info.max_sets,
                                                 .poolSizeCount = static_cast<uint32_t>(info.pool_sizes.size()),
                                                 .pPoolSizes = info.pool_sizes.data()};

  descriptor_pool_ = device.createDescriptorPool(create_info);
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
  const vk::DescriptorSetAllocateInfo alloc_info{
      .descriptorPool = descriptor_pool_, .descriptorSetCount = 1, .pSetLayouts = &layout};

  const auto sets = device_.allocateDescriptorSets(alloc_info);
  return sets.front();
}

std::vector<vk::DescriptorSet> VulkanDescriptorPool::allocate(const std::vector<vk::DescriptorSetLayout> &layouts) const
{
  const vk::DescriptorSetAllocateInfo alloc_info{.descriptorPool = descriptor_pool_,
                                                 .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
                                                 .pSetLayouts = layouts.data()};

  return device_.allocateDescriptorSets(alloc_info);
}

VulkanDescriptorSetLayout::VulkanDescriptorSetLayout(const vk::Device device,
                                                     const std::vector<vk::DescriptorSetLayoutBinding> &bindings,
                                                     const std::vector<vk::DescriptorBindingFlags> &binding_flags,
                                                     const vk::DescriptorSetLayoutCreateFlags flags) : device_(device)
{
  vk::DescriptorSetLayoutBindingFlagsCreateInfo binding_flags_info;
  if (!binding_flags.empty())
  {
    binding_flags_info.bindingCount = static_cast<uint32_t>(binding_flags.size());
    binding_flags_info.pBindingFlags = binding_flags.data();
  }

  const vk::DescriptorSetLayoutCreateInfo create_info{.pNext = !binding_flags.empty() ? &binding_flags_info : nullptr,
                                                      .flags = flags,
                                                      .bindingCount = static_cast<uint32_t>(bindings.size()),
                                                      .pBindings = bindings.data()};

  layout_ = device.createDescriptorSetLayout(create_info);
}

VulkanDescriptorSetLayout::~VulkanDescriptorSetLayout()
{
  if (layout_)
  {
    device_.destroyDescriptorSetLayout(layout_);
  }
}
