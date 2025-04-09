module;

#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

export module Vulkan:Image;

export
{
	struct VulkanImageCreateInfo
	{
		VkImageLayout initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;
		VkFormat format;
		VkExtent3D extent{};
		VkImageUsageFlags usage_flags = 0;
		VkImageAspectFlags aspect_flags = 0;
		VkDescriptorSet descriptor_set = VK_NULL_HANDLE;
	};

	struct VulkanImage
	{
		static VulkanImage Create(const VkDevice& logical_device, const VmaAllocator& allocator, const VulkanImageCreateInfo& create_info);
		static void TransitionLayout(const VkCommandBuffer& cmd_buffer, const VkImage& image, const VkImageLayout& src, const VkImageLayout& dst);

		VmaAllocation allocation = VK_NULL_HANDLE;
		VkSampler sampler = VK_NULL_HANDLE;
		VkImage image = VK_NULL_HANDLE;
		VkImageView view = VK_NULL_HANDLE;
		VkExtent3D extent{};
		VkFormat format{};
	};
}