#include <macros/AurionLog.h>

#include <vector>

#include <vulkan/vulkan.h>

import Vulkan;

VulkanDescriptorSetLayout VulkanDescriptorSetLayout::Create(const VulkanDevice* logical_device, const std::vector<VkDescriptorSetLayoutBinding>& bindings, const VkDescriptorSetLayoutCreateFlags& create_flags, void* extension)
{
	VulkanDescriptorSetLayout layout{
		VK_NULL_HANDLE,
		bindings
	};

	VkDescriptorSetLayoutCreateInfo create_info{};
	create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	create_info.flags = create_flags;
	create_info.pNext = extension;
	create_info.bindingCount = static_cast<uint32_t>(layout.bindings.size());
	create_info.pBindings = layout.bindings.data();

	if (vkCreateDescriptorSetLayout(logical_device->handle, &create_info, nullptr, &layout.handle) != VK_SUCCESS)
		AURION_ERROR("[VulkanDescriptorSet::Create] Failed to create descriptor set layout!");

	return layout;
}

void VulkanDescriptorSetLayout::Destroy(const VulkanDevice* logical_device, VulkanDescriptorSetLayout& layout)
{
	vkDestroyDescriptorSetLayout(logical_device->handle, layout.handle, nullptr);
	layout.bindings.clear();
};

VulkanDescriptorPool VulkanDescriptorPool::Create(const VulkanDevice* logical_device, const uint32_t& max_sets, const std::vector<VkDescriptorPoolSize>& pool_sizes, const VkDescriptorPoolCreateFlags& create_flags, void* extension)
{
	VulkanDescriptorPool pool{};

	VkDescriptorPoolCreateInfo create_info{};
	create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	create_info.flags = create_flags;
	create_info.pNext = extension;
	create_info.maxSets = max_sets;
	create_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
	create_info.pPoolSizes = pool_sizes.data();

	if (vkCreateDescriptorPool(logical_device->handle, &create_info, nullptr, &pool.handle) != VK_SUCCESS)
		AURION_ERROR("[VulkanDescriptorPool::Create] Failed to create descriptor pool!");

	return pool;
}

std::vector<VkDescriptorSet> VulkanDescriptorPool::Allocate(const VulkanDevice* logical_device, const VulkanDescriptorPool& pool, const VulkanDescriptorSetLayout& layout, const uint32_t& count)
{
	if (layout.handle == VK_NULL_HANDLE)
	{
		AURION_ERROR("[VulkanDescriptorPool::Allocate] Failed to allocate descriptor sets: Invalid layout handle!");
		return std::vector<VkDescriptorSet>();
	}

	if (count == 0)
	{
		AURION_ERROR("[VulkanDescriptorPool::Allocate] Failed to allocate descriptor sets: Count must be greater than 0!");
		return std::vector<VkDescriptorSet>();
	}

	std::vector<VkDescriptorSetLayout> layout_copies(count, layout.handle);
	std::vector<VkDescriptorSet> out_sets(count);

	VkDescriptorSetAllocateInfo alloc_info{};
	alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	alloc_info.descriptorPool = pool.handle;
	alloc_info.descriptorSetCount = count;
	alloc_info.pSetLayouts = layout_copies.data();

	if (vkAllocateDescriptorSets(logical_device->handle, &alloc_info, out_sets.data()) != VK_SUCCESS)
	{
		AURION_ERROR("[VulkanDescriptorPool::Allocate] Failed to allocate descriptor sets: vkAllocateDescriptorSets failed!");
		return std::vector<VkDescriptorSet>();
	}

	return out_sets;
}

void VulkanDescriptorPool::Destroy(const VulkanDevice* logical_device, VulkanDescriptorPool& pool)
{
	vkDestroyDescriptorPool(logical_device->handle, pool.handle, nullptr);
}