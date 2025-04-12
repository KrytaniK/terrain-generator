module;

#include <vector>

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
		
		static VkImageMemoryBarrier2 CreateLayoutTransition(
			const VkImage& image, 
			const VkImageLayout& src, 
			const VkImageLayout& dst,
			const VkPipelineStageFlags2& src_stage,
			const VkPipelineStageFlags2& dst_stage,
			const VkAccessFlags2& src_access,
			const VkAccessFlags2& dst_access
		);

		static void TransitionLayouts(const VkCommandBuffer& cmd_buffer, const std::vector<VkImageMemoryBarrier2>& barriers);

		VmaAllocation allocation = VK_NULL_HANDLE;
		VkSampler sampler = VK_NULL_HANDLE;
		VkImage image = VK_NULL_HANDLE;
		VkImageView view = VK_NULL_HANDLE;
		VkExtent3D extent{};
		VkFormat format{};
	};
}