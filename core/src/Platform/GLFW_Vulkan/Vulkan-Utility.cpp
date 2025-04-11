#include <macros/AurionLog.h>

#include <string>

#include <vulkan/vulkan.h>
#include <shaderc/shaderc.hpp>
#include <spirv_cross/spirv_cross.hpp>

import Aurion.FileSystem;
import Vulkan;

namespace Vulkan
{
	VkPipelineShaderStageCreateInfo CreatePipelineShader(
		const VulkanDevice* logical_device,
		const VkShaderStageFlagBits& shader_stage, 
		const VkPipelineShaderStageCreateFlags& create_flags, 
		const std::string& file_path, 
		bool is_hlsl, 
		const char* entry_point
	){
		// Setup shader stage
		VkPipelineShaderStageCreateInfo shader_stage_create_info{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.flags = create_flags,
			.stage = shader_stage,
			.pName = entry_point,
		};

		// NOTE: This is WINDOWS only!!!! 
		Aurion::WindowsFileSystem fs;

		// Ensure the file exists
		if (!fs.FileExists(file_path.c_str()))
		{
			AURION_ERROR("[Vulkan::CreatePipelineShader] Failed to create shader: File not found (%s)", file_path.c_str());
			return shader_stage_create_info;
		}

		// Open the file (don't force create it)
		Aurion::FSFileHandle raw_handle = fs.OpenFile(file_path.c_str(), false);

		// Remove file extension and replace with .spv
		size_t ext_start = file_path.find_last_of('.');
		size_t base_start = file_path.find_last_of('/');

		// Strip extension
		std::string path_no_ext = file_path.substr(0, ext_start);

		// Strip path to get file name
		std::string file_name = path_no_ext.substr(base_start);

		// Configure the local cache path for the SPIR-V binary of this shader
		//	NOTE: CACHE PATH SHOULD BE DYNAMIC, NOT A MAGIC VALUE!
		std::string local_cache_path = "assets/shaders/cached/" + file_name + ".spv";

		// If the SPIR-V binary exists
		if (fs.FileExists(local_cache_path.c_str()))
		{
			// Load the SPIR-V binary
			Aurion::FSFileHandle spv_handle = fs.OpenFile(local_cache_path.c_str(), false);
			uint32_t* binary_data = (uint32_t*)spv_handle.Read();
			size_t binary_size = spv_handle.GetSize() - 1;

			// Generate Shader Module
			{
				VkShaderModuleCreateInfo shader_module_info{};
				shader_module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
				shader_module_info.codeSize = binary_size;
				shader_module_info.pCode = binary_data;

				// Attempt to generate the shader module
				if (vkCreateShaderModule(logical_device->handle, &shader_module_info, nullptr, &shader_stage_create_info.module) != VK_SUCCESS)
				{
					AURION_ERROR("[Vulkan::CreatePipelineShader] Failed to create shader: VkShaderModule creation failed.");
					return shader_stage_create_info;
				}
			}

			// Compile and replace the cached version
			//	TODO: Compilation takes a while (~300+ ms), so spinning up another thread for this would be ideal
			//	NOTE: This is dead code right now. Recompilation will be supported later. To force the recompilation
			//			of a shader, the .spv file must be manually deleted
			if (false)
			{
				shaderc::Compiler compiler;
				shaderc::CompileOptions options;

				options.SetOptimizationLevel(shaderc_optimization_level_performance);
				options.SetSourceLanguage(is_hlsl ? shaderc_source_language_hlsl : shaderc_source_language_glsl);

				shaderc::SpvCompilationResult spv_compiled = compiler.CompileGlslToSpv(
					(const char*)raw_handle.Read(),
					ShaderStageToShadercKind(shader_stage),
					raw_handle.GetInfo().name,
					options
				);

				if (spv_compiled.GetCompilationStatus() != shaderc_compilation_status_success) {
					AURION_ERROR("[Vulkan::CreatePipelineShader] Shaderc Compilation Error: %s", spv_compiled.GetErrorMessage().c_str());
					return shader_stage_create_info;
				}

				// Once successfully compiled, write to the cached location
				std::vector<uint32_t> compilation_result(spv_compiled.begin(), spv_compiled.end());

				// Overwrite existing compiled binary
				spv_handle.Write(compilation_result.data(), compilation_result.size() * sizeof(uint32_t), 0);
			}

			return shader_stage_create_info;
		}

		// If the SPIR-V binary doesn't exist:

		// First, generate the file handle for the desired directory
		Aurion::FSFileHandle spv_handle = fs.OpenFile(local_cache_path.c_str(), true);

		// Compile the raw shader into SPIR-V
		shaderc::Compiler compiler;
		shaderc::CompileOptions options;

		options.SetOptimizationLevel(shaderc_optimization_level_performance);
		options.SetSourceLanguage(is_hlsl ? shaderc_source_language_hlsl : shaderc_source_language_glsl);

		shaderc::SpvCompilationResult spv_compiled = compiler.CompileGlslToSpv(
			(const char*)raw_handle.Read(),
			ShaderStageToShadercKind(shader_stage),
			raw_handle.GetInfo().name,
			options
		);

		if (spv_compiled.GetCompilationStatus() != shaderc_compilation_status_success) {
			AURION_ERROR("[Vulkan::CreatePipelineShader] Shaderc Compilation Error: %s", spv_compiled.GetErrorMessage().c_str());
			return shader_stage_create_info;
		}

		// Convert to vector for ease of use
		std::vector<uint32_t> compilation_result(spv_compiled.begin(), spv_compiled.end());

		// After compilation succeeds, generate the shader module
		VkShaderModuleCreateInfo shader_module_info{};
		shader_module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		shader_module_info.codeSize = compilation_result.size() * sizeof(uint32_t);
		shader_module_info.pCode = compilation_result.data();

		// Attempt to generate the shader module
		if (vkCreateShaderModule(logical_device->handle, &shader_module_info, nullptr, &shader_stage_create_info.module) != VK_SUCCESS)
		{
			AURION_ERROR("[Vulkan::CreatePipelineShader] Failed to create shader: VkShaderModule creation failed.");
			return shader_stage_create_info;
		}

		// Then, cache the shader binary on disk
		spv_handle.Write(compilation_result.data(), compilation_result.size() * sizeof(uint32_t), 0);

		// And copy the shader stage info into the current configuration
		return shader_stage_create_info;
	}

	shaderc_shader_kind ShaderStageToShadercKind(const VkShaderStageFlags& flags)
	{
		switch (flags)
		{
			case VK_SHADER_STAGE_VERTEX_BIT: return shaderc_shader_kind::shaderc_vertex_shader;
			case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT: return shaderc_shader_kind::shaderc_tess_control_shader;
			case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT: return shaderc_shader_kind::shaderc_tess_evaluation_shader;
			case VK_SHADER_STAGE_GEOMETRY_BIT: return shaderc_shader_kind::shaderc_geometry_shader;
			case VK_SHADER_STAGE_FRAGMENT_BIT: return shaderc_shader_kind::shaderc_fragment_shader;
			case VK_SHADER_STAGE_COMPUTE_BIT: return shaderc_shader_kind::shaderc_compute_shader;
			case VK_SHADER_STAGE_RAYGEN_BIT_KHR: return shaderc_shader_kind::shaderc_raygen_shader;
			case VK_SHADER_STAGE_ANY_HIT_BIT_KHR: return shaderc_shader_kind::shaderc_anyhit_shader;
			case VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR: return shaderc_shader_kind::shaderc_closesthit_shader;
			case VK_SHADER_STAGE_MISS_BIT_KHR: return shaderc_shader_kind::shaderc_miss_shader;
			case VK_SHADER_STAGE_INTERSECTION_BIT_KHR: return shaderc_shader_kind::shaderc_intersection_shader;
			case VK_SHADER_STAGE_CALLABLE_BIT_KHR: return shaderc_shader_kind::shaderc_callable_shader;
			case VK_SHADER_STAGE_TASK_BIT_EXT: return shaderc_shader_kind::shaderc_task_shader;
			case VK_SHADER_STAGE_MESH_BIT_EXT: return shaderc_shader_kind::shaderc_mesh_shader;
			default: return shaderc_shader_kind::shaderc_vertex_shader;
		}

		return shaderc_shader_kind::shaderc_vertex_shader;
	}
}