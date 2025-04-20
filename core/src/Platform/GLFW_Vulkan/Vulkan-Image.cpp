#include <vector>

#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

import Vulkan;

VulkanImage VulkanImage::Create(const VkDevice& logical_device, const VmaAllocator& allocator, const VulkanImageCreateInfo& create_info)
{
	VulkanImage out_image;

	out_image.sample_flags = create_info.msaa_samples;
	out_image.image_aspect = create_info.aspect_flags;

	out_image.extent = create_info.extent;
	out_image.format = create_info.format;

	// Image Creation
	VkImageCreateInfo imageCreateInfo{};
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.pNext = nullptr;

	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.initialLayout = create_info.initial_layout;

	imageCreateInfo.format = create_info.format;
	imageCreateInfo.extent = create_info.extent;

	imageCreateInfo.mipLevels = 1;
	imageCreateInfo.arrayLayers = 1;

	// For MSAA
	imageCreateInfo.samples = create_info.msaa_samples;

	// Tiling / Usage
	imageCreateInfo.tiling = create_info.tiling;
	imageCreateInfo.usage = create_info.usage_flags;

	// Allocate image from gpu local memory
	VmaAllocationCreateInfo imgAllocInfo{};
	imgAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	imgAllocInfo.requiredFlags = create_info.memory_props;

	vmaCreateImage(allocator, &imageCreateInfo, &imgAllocInfo, &out_image.image, &out_image.allocation, nullptr);

	// Image View Creation
	VkImageViewCreateInfo viewCreateInfo{};
	viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewCreateInfo.pNext = nullptr;

	viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewCreateInfo.image = out_image.image;
	viewCreateInfo.format = out_image.format;
	viewCreateInfo.subresourceRange.baseMipLevel = 0;
	viewCreateInfo.subresourceRange.levelCount = 1;
	viewCreateInfo.subresourceRange.baseArrayLayer = 0;
	viewCreateInfo.subresourceRange.layerCount = 1;
	viewCreateInfo.subresourceRange.aspectMask = create_info.aspect_flags;

	vkCreateImageView(logical_device, &viewCreateInfo, nullptr, &out_image.view);

	// Create Sampler
	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT; // outside image bounds just use border color
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT; 
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.minLod = -1000;
	samplerInfo.maxLod = 1000;
	samplerInfo.maxAnisotropy = 1.0f;

	vkCreateSampler(logical_device, &samplerInfo, nullptr, &out_image.sampler);

	return out_image;
}

void VulkanImage::Destroy(const VulkanDevice* logical_device, const VulkanImage& image)
{
	vkDestroySampler(logical_device->handle, image.sampler, nullptr);
	vkDestroyImageView(logical_device->handle, image.view, nullptr);
	vmaDestroyImage(logical_device->allocator, image.image, image.allocation);
}

VkImageMemoryBarrier2 VulkanImage::CreateLayoutTransition(const VulkanImage& image, const VkImageLayout& src, const VkImageLayout& dst, const VkPipelineStageFlags2& src_stage, const VkPipelineStageFlags2& dst_stage, const VkAccessFlags2& src_access, const VkAccessFlags2& dst_access)
{
	VkImageMemoryBarrier2 barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
	barrier.pNext = nullptr;

	// This is a bit general, and not at all optimal. Future TODO: Allow custom masks
	barrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
	barrier.srcAccessMask = VK_ACCESS_2_MEMORY_READ_BIT;
	barrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
	barrier.dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;

	// Transition from old to new layout
	barrier.oldLayout = src;
	barrier.newLayout = dst;

	// Create Image Subresource Range with aspect mask
	VkImageSubresourceRange sub_image{};
	sub_image.aspectMask = image.image_aspect;
	sub_image.baseMipLevel = 0;
	sub_image.levelCount = VK_REMAINING_MIP_LEVELS;
	sub_image.baseArrayLayer = 0;
	sub_image.layerCount = VK_REMAINING_ARRAY_LAYERS;

	// Create aspect mask
	barrier.subresourceRange = sub_image;
	barrier.image = image.image;

	// Dependency struct
	VkDependencyInfo dep_info{};
	dep_info.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
	dep_info.pNext = nullptr;

	// Attach image barrier
	dep_info.imageMemoryBarrierCount = 1;
	dep_info.pImageMemoryBarriers = &barrier;

	return barrier;
}

VkImageMemoryBarrier2 VulkanImage::CreateLayoutTransition(const VkImage& image, const VkImageLayout& src, const VkImageLayout& dst, const VkPipelineStageFlags2& src_stage, const VkPipelineStageFlags2& dst_stage, const VkAccessFlags2& src_access, const VkAccessFlags2& dst_access)
{
	VkImageMemoryBarrier2 barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
	barrier.pNext = nullptr;

	// This is a bit general, and not at all optimal. Future TODO: Allow custom masks
	barrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
	barrier.srcAccessMask = VK_ACCESS_2_MEMORY_READ_BIT;
	barrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
	barrier.dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;

	// Transition from old to new layout
	barrier.oldLayout = src;
	barrier.newLayout = dst;

	// Create Image Subresource Range with aspect mask
	VkImageSubresourceRange sub_image{};
	sub_image.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	sub_image.baseMipLevel = 0;
	sub_image.levelCount = VK_REMAINING_MIP_LEVELS;
	sub_image.baseArrayLayer = 0;
	sub_image.layerCount = VK_REMAINING_ARRAY_LAYERS;

	// Create aspect mask
	barrier.subresourceRange = sub_image;
	barrier.image = image;

	// Dependency struct
	VkDependencyInfo dep_info{};
	dep_info.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
	dep_info.pNext = nullptr;

	// Attach image barrier
	dep_info.imageMemoryBarrierCount = 1;
	dep_info.pImageMemoryBarriers = &barrier;

	return barrier;
}

void VulkanImage::TransitionLayouts(const VkCommandBuffer& cmd_buffer, const std::vector<VkImageMemoryBarrier2>& barriers)
{
	// Dependency struct
	VkDependencyInfo dep_info{};
	dep_info.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
	dep_info.pNext = nullptr;

	// Attach image barrier
	dep_info.imageMemoryBarrierCount = static_cast<uint32_t>(barriers.size());
	dep_info.pImageMemoryBarriers = barriers.data();

	vkCmdPipelineBarrier2(cmd_buffer, &dep_info);
}

bool VulkanImage::HasStencilComponent(const VkFormat& format)
{
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}
