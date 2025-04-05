#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

import Vulkan;

VulkanImage VulkanImage::Create(const VkDevice& logical_device, const VmaAllocator& allocator, const VulkanImageCreateInfo& create_info)
{
	VulkanImage out_image;

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
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;

	// Optimal tiling
	imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCreateInfo.usage = create_info.usage_flags;

	// Allocate image from gpu local memory
	VmaAllocationCreateInfo imgAllocInfo{};
	imgAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	imgAllocInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

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