module;

#include <vulkan/vulkan.h>

export module Vulkan:Buffer;

import :Device;

export
{
	struct VulkanBuffer
	{
		static VulkanBuffer Create(const VulkanDevice* logical_device, const VkDeviceSize& size, const VkBufferUsageFlags& usage, const VkSharingMode& sharing_mode);
		static void Destroy(const VulkanDevice* logical_device, const VulkanBuffer& buffer);

		static VkMemoryRequirements GetMemoryRequirements(const VulkanDevice* logical_device, VulkanBuffer& buffer);

		static uint32_t FindMemoryType(const VulkanDevice* logical_device, VulkanBuffer& buffer, const uint32_t& type_filter, const VkMemoryPropertyFlags& properties);

		static void Allocate(const VulkanDevice* logical_device, VulkanBuffer& buffer, const VkMemoryRequirements& memory_requirements, const uint32_t& memory_type);

		static void Map(const VulkanDevice* logical_device, VulkanBuffer& buffer, const VkDeviceSize& offset, const VkDeviceSize& size, const VkMemoryMapFlags& map_flags, void* data);

		static void Copy(
			const VulkanDevice* logical_device,
			const VkCommandBuffer& command_buffer,
			const VulkanBuffer& src,
			const VulkanBuffer& dst,
			const VkDeviceSize& src_offset,
			const VkDeviceSize& dst_offset,
			const VkDeviceSize& size
		);

		VkBuffer handle = VK_NULL_HANDLE;
		VkDeviceSize size = 0;
		VkDeviceMemory memory = VK_NULL_HANDLE;
	};
}