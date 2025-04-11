module;

#include <string>

#include <vulkan/vulkan.h>
#include <shaderc/shaderc.h>

export module Vulkan:Utility;

import :Device;

export namespace Vulkan
{
	VkPipelineShaderStageCreateInfo CreatePipelineShader(
		const VulkanDevice* logical_device,
		const VkShaderStageFlagBits& shader_stage, 
		const VkPipelineShaderStageCreateFlags& create_flags,
		const std::string& file_path, 
		bool is_hlsl = false, 
		const char* entry_point = "main"
	);

	shaderc_shader_kind ShaderStageToShadercKind(const VkShaderStageFlags& flags);
}