#include "vk_descriptor.hpp"

#include "util/print.hpp"
#include "util/vk_check.hpp"

VulkanDescriptorPool::VulkanDescriptorPool(VkDevice device,
                                           const DescriptorPoolInfo &info)
    : m_device(device) {
  VkDescriptorPoolCreateInfo createInfo{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .pNext = nullptr,
      .flags = info.flags,
      .maxSets = info.maxSets,
      .poolSizeCount = static_cast<uint32_t>(info.poolSizes.size()),
      .pPoolSizes = info.poolSizes.data()};

  VK_CHECK(
      vkCreateDescriptorPool(device, &createInfo, nullptr, &m_descriptorPool));
  Util::println("Created descriptor pool");
}

VulkanDescriptorPool::~VulkanDescriptorPool() {
  if (m_descriptorPool != VK_NULL_HANDLE) {
    vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
  }
  Util::println("Destroyed descriptor pool");
}

VkDescriptorSet VulkanDescriptorPool::allocate(VkDescriptorSetLayout layout) {
  VkDescriptorSetAllocateInfo allocInfo{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .pNext = nullptr,
      .descriptorPool = m_descriptorPool,
      .descriptorSetCount = 1,
      .pSetLayouts = &layout};

  VkDescriptorSet set = nullptr;
  VK_CHECK(vkAllocateDescriptorSets(m_device, &allocInfo, &set));
  return set;
}

std::vector<VkDescriptorSet> VulkanDescriptorPool::allocate(
    const std::vector<VkDescriptorSetLayout> &layouts) {
  VkDescriptorSetAllocateInfo allocInfo{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .pNext = nullptr,
      .descriptorPool = m_descriptorPool,
      .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
      .pSetLayouts = layouts.data()};

  std::vector<VkDescriptorSet> sets;
  VK_CHECK(vkAllocateDescriptorSets(m_device, &allocInfo, sets.data()));
  return sets;
}

VulkanDescriptorSetLayout::VulkanDescriptorSetLayout(
    VkDevice device, const std::vector<VkDescriptorSetLayoutBinding> &bindings)
    : m_device(device) {
  VkDescriptorSetLayoutCreateInfo createInfo{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0U,
      .bindingCount = static_cast<uint32_t>(bindings.size()),
      .pBindings = bindings.data()};

  VK_CHECK(
      vkCreateDescriptorSetLayout(device, &createInfo, nullptr, &m_layout));
}

VulkanDescriptorSetLayout::~VulkanDescriptorSetLayout() {
  if (m_layout != VK_NULL_HANDLE) {
    vkDestroyDescriptorSetLayout(m_device, m_layout, nullptr);
  }
}