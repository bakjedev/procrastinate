#pragma once
#include <vector>
#include <vulkan/vulkan.hpp>

struct DescriptorPoolInfo
{
  vk::DescriptorPoolCreateFlags flags;
  uint32_t max_sets;
  std::vector<vk::DescriptorPoolSize> pool_sizes;
};

class VulkanDescriptorPool
{
public:
  VulkanDescriptorPool(vk::Device device, const DescriptorPoolInfo &info);
  VulkanDescriptorPool(const VulkanDescriptorPool &) = delete;
  VulkanDescriptorPool(VulkanDescriptorPool &&) = delete;
  VulkanDescriptorPool &operator=(const VulkanDescriptorPool &) = delete;
  VulkanDescriptorPool &operator=(VulkanDescriptorPool &&) = delete;
  ~VulkanDescriptorPool();

  vk::DescriptorSet allocate(vk::DescriptorSetLayout layout) const;
  std::vector<vk::DescriptorSet> allocate(const std::vector<vk::DescriptorSetLayout> &layouts) const;

  vk::DescriptorPool get() const { return descriptor_pool_; }

private:
  vk::Device device_;
  vk::DescriptorPool descriptor_pool_;
};

class VulkanDescriptorSetLayout
{
public:
  VulkanDescriptorSetLayout(vk::Device device, const std::vector<vk::DescriptorSetLayoutBinding> &bindings,
                            const std::vector<vk::DescriptorBindingFlags> &binding_flags = {},
                            vk::DescriptorSetLayoutCreateFlags flags = {});
  VulkanDescriptorSetLayout(const VulkanDescriptorSetLayout &) = delete;
  VulkanDescriptorSetLayout(VulkanDescriptorSetLayout &&) = delete;
  VulkanDescriptorSetLayout &operator=(const VulkanDescriptorSetLayout &) = delete;
  VulkanDescriptorSetLayout &operator=(VulkanDescriptorSetLayout &&) = delete;
  ~VulkanDescriptorSetLayout();

  [[nodiscard]] vk::DescriptorSetLayout get() const { return layout_; }

private:
  vk::Device device_;
  vk::DescriptorSetLayout layout_;
};
