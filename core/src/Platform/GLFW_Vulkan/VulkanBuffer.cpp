#include <macros/AurionLog.h>

#include <cstring>

#include <vulkan/vulkan.h>

import Vulkan;

VulkanBuffer VulkanBuffer::Create(const VulkanDevice* logical_device, const VkDeviceSize& size, const VkBufferUsageFlags& usage, const VkSharingMode& sharing_mode)
{
	VulkanBuffer buffer{};

	VkBufferCreateInfo create_info{};
	create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	create_info.size = size;
	create_info.usage = usage;
	create_info.sharingMode = sharing_mode;

	if (vkCreateBuffer(logical_device->handle, &create_info, nullptr, &buffer.handle) != VK_SUCCESS)
	{
		AURION_ERROR("[VulkanBuffer::Create] Failed to create vulkan buffer!");
		return buffer;
	}

	buffer.size = size;

	return buffer;
}

void VulkanBuffer::Destroy(const VulkanDevice* logical_device, const VulkanBuffer& buffer)
{
	vkDestroyBuffer(logical_device->handle, buffer.handle, nullptr);
	vkFreeMemory(logical_device->handle, buffer.memory, nullptr);
}

VkMemoryRequirements VulkanBuffer::GetMemoryRequirements(const VulkanDevice* logical_device, VulkanBuffer& buffer)
{
	VkMemoryRequirements requirements{};
	vkGetBufferMemoryRequirements(logical_device->handle, buffer.handle, &requirements);
	return requirements;
}

uint32_t VulkanBuffer::FindMemoryType(const VulkanDevice* logical_device, VulkanBuffer& buffer, const uint32_t& type_filter, const VkMemoryPropertyFlags& properties)
{
	VkPhysicalDeviceMemoryProperties memory_properties;
	vkGetPhysicalDeviceMemoryProperties(logical_device->physical_device, &memory_properties);

	// Search for a suitable memory type, fitting the provided type filter and desired properties.
	for (uint32_t i = 0; i < memory_properties.memoryTypeCount; i++)
		if (type_filter & (1 << i) && (memory_properties.memoryTypes[i].propertyFlags & properties))
			return i;

	AURION_ERROR("[VulkanBuffer::FindMemoryType] Failed to find memory type: $d", type_filter);

	return VK_MEMORY_PROPERTY_FLAG_BITS_MAX_ENUM;
}

void VulkanBuffer::Allocate(const VulkanDevice* logical_device, VulkanBuffer& buffer, const VkMemoryRequirements& memory_requirements, const uint32_t& memory_type)
{
	VkMemoryAllocateInfo allocation_info{};
	allocation_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocation_info.allocationSize = memory_requirements.size;
	allocation_info.memoryTypeIndex = memory_type;

	if (vkAllocateMemory(logical_device->handle, &allocation_info, nullptr, &buffer.memory) != VK_SUCCESS)
	{
		AURION_ERROR("[VulkanBuffer::Allocate] Failed to allocate buffer memory!");
		return;
	}

	// Bind buffer memory if allocation was successful
	if (vkBindBufferMemory(logical_device->handle, buffer.handle, buffer.memory, 0) != VK_SUCCESS)
	{
		AURION_ERROR("[VulkanBuffer::Allocate] Failed to bind buffer memory!");
		return;
	}
}

void VulkanBuffer::Map(const VulkanDevice* logical_device, VulkanBuffer& buffer, const VkDeviceSize& offset, const VkDeviceSize& size, const VkMemoryMapFlags& map_flags, void* data)
{
	void* mapped = nullptr;
	if (vkMapMemory(logical_device->handle, buffer.memory, offset, size, map_flags, &mapped) != VK_SUCCESS)
	{
		AURION_ERROR("[VulkanBuffer::Map] Failed to map buffer memory!");
		return;
	}

	std::memcpy(mapped, data, (size_t)size);
	vkUnmapMemory(logical_device->handle, buffer.memory);
}

void VulkanBuffer::Copy(
	const VulkanDevice* logical_device, 
	const VkCommandBuffer& command_buffer, 
	const VulkanBuffer& src, 
	const VulkanBuffer& dst, 
	const VkDeviceSize& src_offset, 
	const VkDeviceSize& dst_offset, 
	const VkDeviceSize& size
){
	VkBufferCopy copyRegion{};
	copyRegion.srcOffset = src_offset;
	copyRegion.dstOffset = dst_offset;
	copyRegion.size = size;
	vkCmdCopyBuffer(command_buffer, src.handle, dst.handle, 1, &copyRegion);
}
