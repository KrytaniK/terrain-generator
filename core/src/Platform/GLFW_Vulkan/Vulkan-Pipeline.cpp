#include <macros/AurionLog.h>

#include <string>
#include <vector>
#include <cstdint>
#include <thread>

#include <shaderc/shaderc.hpp>
#include <spirv_cross/spirv_cross.hpp>
#include <vulkan/vulkan.h>

import Vulkan;
import Aurion.FileSystem;

VulkanPipelineBuilder::VulkanPipelineBuilder()
{

}

VulkanPipelineBuilder::~VulkanPipelineBuilder()
{
	this->Cleanup();
}

void VulkanPipelineBuilder::Initialize(VulkanDevice* device, std::vector<VulkanPipeline>& pipeline_buffer)
{
	m_logical_device = device;
	m_pipeline_buffer = &pipeline_buffer;
}

void VulkanPipelineBuilder::Cleanup()
{
	for (size_t i = 0; i < m_configurations.size(); i++)
	{
		VulkanPipelineConfiguration& config = m_configurations[i];

		// Clean up any dangling shader modules
		for (auto& stage : config.shader_stages)
			vkDestroyShaderModule(m_logical_device->handle, stage.module, nullptr);
		config.shader_stages.clear();

		// Cleanup any layout that might exist
		switch (config.type)
		{
			case VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO:
			{
				if(config.compute_info.layout != VK_NULL_HANDLE)
					vkDestroyPipelineLayout(m_logical_device->handle, config.compute_info.layout, nullptr);
				break;
			}
			case VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO:
			{
				if (config.graphics_info.layout != VK_NULL_HANDLE)
					vkDestroyPipelineLayout(m_logical_device->handle, config.graphics_info.layout, nullptr);
				break;
			}
			case VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR:
			{
				if (config.raytracing_info.layout != VK_NULL_HANDLE)
					vkDestroyPipelineLayout(m_logical_device->handle, config.raytracing_info.layout, nullptr);
				break;
			}
			default:
				continue;
		}

		// Clean up any dangling descriptor set layouts
		for (size_t j = 0; j < config.ds_layouts.size(); j++)
			if (config.ds_layouts[j] != VK_NULL_HANDLE)
				vkDestroyDescriptorSetLayout(m_logical_device->handle, config.ds_layouts[j], nullptr);
		config.ds_layouts.clear();
	}
}

VulkanPipelineBuilder& VulkanPipelineBuilder::Configure(const VkStructureType& pipeline_type, const VkPipelineCreateFlags& create_flags)
{
	// Generate a new pipeline configuration
	m_configurations.emplace_back(VulkanPipelineConfiguration{
		.type = pipeline_type
	});

	// Validate pipeline structure parameter and set relevant info structure type
	switch (pipeline_type)
	{
		case VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO: 
		{
			m_configurations.back().compute_info.sType = pipeline_type;
			m_configurations.back().compute_info.flags = create_flags;
			break;
		}
		case VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO:
		{
			m_configurations.back().graphics_info.sType = pipeline_type;
			m_configurations.back().graphics_info.flags = create_flags;
			break;
		}
		case VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR:
		{
			m_configurations.back().raytracing_info.sType = pipeline_type;
			m_configurations.back().raytracing_info.flags = create_flags;
			break;
		}
		default: 
			AURION_WARN("[Vulkan Pipeline Builder] Starting a configuration chain with unsupported pipeline type: %d!", pipeline_type);
	}

	return *this;
}

VulkanPipelineBuilder::Result VulkanPipelineBuilder::Build()
{
	std::vector<VkPipeline> compute_pipeline_refs;
	std::vector<VkPipeline> graphics_pipeline_refs;
	std::vector<VkPipeline> raytracing_pipeline_refs;
	std::vector<VkComputePipelineCreateInfo> compute_creates;
	std::vector<VkGraphicsPipelineCreateInfo> graphics_creates;
	std::vector<VkRayTracingPipelineCreateInfoKHR> raytracing_creates;

	// Setup pipeline buffers for each type
	for (size_t i = 0; i < m_configurations.size(); i++)
	{
		// Create a pipeline in the buffer for each configuration
		m_pipeline_buffer->emplace_back(VulkanPipeline{});
		VulkanPipeline& pipeline = m_pipeline_buffer->back();

		// Grab an easy ref to the current configuration
		VulkanPipelineConfiguration& config = m_configurations[i];

		switch (config.type)
		{
			case VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO:
			{
				// Attach shader stages
				config.compute_info.stage = config.shader_stages[0];

				// Grab references
				compute_creates.emplace_back(config.compute_info);
				compute_pipeline_refs.emplace_back(VkPipeline{});

				// Copy pipeline data
				pipeline.layout = config.compute_info.layout;
				pipeline.ds_layouts = config.ds_layouts;
				pipeline.push_constants = config.push_constants;

				// Reference in build result
				m_build_result.compute_pipelines.emplace_back(&pipeline);

				break;
			}
			case VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO:
			{
				// Attach all graphics configuration data
				config.graphics_info.pVertexInputState = &config.vertex_input_state;
				config.graphics_info.pInputAssemblyState = &config.input_assembly_state;
				config.graphics_info.pTessellationState = &config.tessellation_state;
				config.graphics_info.pViewportState = &config.viewport_state;
				config.graphics_info.pRasterizationState = &config.rasterization_state;
				config.graphics_info.pMultisampleState = &config.multisample_state;
				config.graphics_info.pDepthStencilState = &config.depth_stencil_state;
				config.graphics_info.pColorBlendState = &config.color_blend_state;
				config.graphics_info.pDynamicState = &config.dynamic_state;

				// For pipeline derivation (unused)
				config.graphics_info.basePipelineHandle = VK_NULL_HANDLE;
				config.graphics_info.basePipelineIndex = -1;

				// Attach dynamic rendering information (if using)
				if (config.use_dynamic_rendering)
					config.graphics_info.pNext = &config.dynamic_render_info;

				// Attach shader stages
				config.graphics_info.stageCount = static_cast<uint32_t>(config.shader_stages.size());
				config.graphics_info.pStages = config.shader_stages.data();

				// Grab references
				graphics_creates.emplace_back(config.graphics_info);
				graphics_pipeline_refs.emplace_back(VkPipeline{});

				// Copy pipeline data
				pipeline.layout = config.graphics_info.layout;
				pipeline.render_pass = config.graphics_info.renderPass;
				pipeline.ds_layouts = config.ds_layouts;
				pipeline.push_constants = config.push_constants;

				// Reference in build result
				m_build_result.graphics_pipelines.emplace_back(&pipeline);

				break;
			}
			case VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR:
			{
				// Attach shader stages
				config.raytracing_info.stageCount = static_cast<uint32_t>(config.shader_stages.size());
				config.raytracing_info.pStages = config.shader_stages.data();

				// Grab references
				raytracing_creates.emplace_back(config.raytracing_info);
				raytracing_pipeline_refs.emplace_back(VkPipeline{});

				// Copy pipeline data
				pipeline.layout = config.raytracing_info.layout;
				pipeline.ds_layouts = config.ds_layouts;
				pipeline.push_constants = config.push_constants;

				// Reference in build result
				m_build_result.ray_tracing_pipelines.emplace_back(&pipeline);

				break;
			}
			default: continue;
		}
	}

	// Batch generate all compute pipelines
	if (compute_creates.size() > 0)
		vkCreateComputePipelines(m_logical_device->handle, VK_NULL_HANDLE, static_cast<uint32_t>(compute_creates.size()), compute_creates.data(), nullptr, compute_pipeline_refs.data());

	// Batch generate all graphics pipelines
	if (graphics_creates.size() > 0)
		vkCreateGraphicsPipelines(m_logical_device->handle, VK_NULL_HANDLE, static_cast<uint32_t>(graphics_creates.size()), graphics_creates.data(), nullptr, graphics_pipeline_refs.data());

	// Batch generate all ray tracing pipelines (Requires Vulkan Extension)
	//if (raytracing_creates.size() > 0)
		//vkCreateRayTracingPipelinesKHR(m_logical_device->handle, VK_NULL_HANDLE, VK_NULL_HANDLE, static_cast<uint32_t>(raytracing_creates.size()), raytracing_creates.data(), nullptr, raytracing_pipeline_refs.data());

	// Populate pipeline handles
	for (size_t i = 0; i < compute_creates.size(); i++)
	{
		for (size_t j = 0; j < m_pipeline_buffer->size(); j++)
		{
			VulkanPipeline& pipeline = (*m_pipeline_buffer)[j];
			if (pipeline.layout == compute_creates[i].layout)
				pipeline.handle = compute_pipeline_refs[i];
		}
	}

	for (size_t i = 0; i < graphics_creates.size(); i++)
	{
		for (size_t j = 0; j < m_pipeline_buffer->size(); j++)
		{
			VulkanPipeline& pipeline = (*m_pipeline_buffer)[j];
			if (pipeline.layout == graphics_creates[i].layout)
				pipeline.handle = graphics_pipeline_refs[i];
		}
	}

	for (size_t i = 0; i < raytracing_creates.size(); i++)
	{
		for (size_t j = 0; j < m_pipeline_buffer->size(); j++)
		{
			VulkanPipeline& pipeline = (*m_pipeline_buffer)[j];
			if (pipeline.layout == raytracing_creates[i].layout)
				pipeline.handle = raytracing_pipeline_refs[i];
		}
	}

	// Destroy all shader modules
	for (size_t i = 0; i < m_configurations.size(); i++)
	{
		VulkanPipelineConfiguration& config = m_configurations[i];

		for (size_t j = 0; j < config.shader_stages.size(); j++)
			vkDestroyShaderModule(m_logical_device->handle, config.shader_stages[j].module, nullptr);
	}

	// Reset configuration state
	m_configurations.clear();

	// Return result
	return m_build_result;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::BindShader(const VkShaderStageFlagBits& shader_stage, const VkPipelineShaderStageCreateFlags& create_flags, const std::string& file_path, bool is_hlsl, const char* entry_point)
{
	// Grab the current configuration
	VulkanPipelineConfiguration& config = m_configurations.back();

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
		AURION_ERROR("[Vulkan Pipeline Builder] Failed to bind shader: File not found (%s)", file_path.c_str());
		return *this;
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
			if (vkCreateShaderModule(m_logical_device->handle, &shader_module_info, nullptr, &shader_stage_create_info.module) != VK_SUCCESS)
			{
				AURION_ERROR("[Vulkan Pipeline Builder] Faile d to bind shader: VkShaderModule creation failed.");
				return *this;
			}

			// Copy the shader stage info into the current configuration
			config.shader_stages.emplace_back(shader_stage_create_info);
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
				VkShaderToShaderc(shader_stage),
				raw_handle.GetInfo().name,
				options
			);

			if (spv_compiled.GetCompilationStatus() != shaderc_compilation_status_success) {
				AURION_ERROR("Shaderc Compilation Error: %s", spv_compiled.GetErrorMessage().c_str());
				return *this;
			}

			// Once successfully compiled, write to the cached location
			std::vector<uint32_t> compilation_result(spv_compiled.begin(), spv_compiled.end());

			// Overwrite existing compiled binary
			spv_handle.Write(compilation_result.data(), compilation_result.size() * sizeof(uint32_t), 0);
		}

		return *this;
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
		VkShaderToShaderc(shader_stage),
		raw_handle.GetInfo().name,
		options
	);

	if (spv_compiled.GetCompilationStatus() != shaderc_compilation_status_success) {
		AURION_ERROR("Shaderc Compilation Error: %s", spv_compiled.GetErrorMessage().c_str());
		return *this;
	}

	// Convert to vector for ease of use
	std::vector<uint32_t> compilation_result(spv_compiled.begin(), spv_compiled.end());

	// After compilation succeeds, generate the shader module
	VkShaderModuleCreateInfo shader_module_info{};
	shader_module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shader_module_info.codeSize = compilation_result.size() * sizeof(uint32_t);
	shader_module_info.pCode = compilation_result.data();

	// Attempt to generate the shader module
	if (vkCreateShaderModule(m_logical_device->handle, &shader_module_info, nullptr, &shader_stage_create_info.module) != VK_SUCCESS)
	{
		AURION_ERROR("[Vulkan Pipeline Builder] Faile d to bind shader: VkShaderModule creation failed.");
		return *this;
	}

	// Then, cache the shader binary on disk
	spv_handle.Write(compilation_result.data(), compilation_result.size() * sizeof(uint32_t), 0);

	// And copy the shader stage info into the current configuration
	config.shader_stages.emplace_back(shader_stage_create_info);

	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::ConfigurePipelineLayout(const VkPipelineLayoutCreateFlags& create_flags)
{
	// Grab the current configuration
	VulkanPipelineConfiguration& config = m_configurations.back();

	config.layout_info = VkPipelineLayoutCreateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.flags = create_flags,
		.setLayoutCount = 0,
		.pSetLayouts = nullptr,
		.pushConstantRangeCount = 0,
		.pPushConstantRanges = nullptr
	};

	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::ConfigureDescSetLayout(const VkDescriptorSetLayoutCreateFlags& create_flags)
{
	// Grab the current configuration
	VulkanPipelineConfiguration& config = m_configurations.back();

	// Generate new handle
	config.ds_layouts.emplace_back(VkDescriptorSetLayout{});

	// Generate new create info struct
	config.ds_layout_info = VkDescriptorSetLayoutCreateInfo{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.flags = create_flags
	};

	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::AddDescSetLayoutBinding(const uint32_t& binding, const VkDescriptorType& desc_type, const uint32_t& count, const VkShaderStageFlags& shader_stages, const VkSampler* immutable_sampler)
{
	// Grab the current configuration
	VulkanPipelineConfiguration& config = m_configurations.back();

	config.bindings.emplace_back(VkDescriptorSetLayoutBinding{
		.binding = binding,
		.descriptorType = desc_type,
		.descriptorCount = count,
		.stageFlags = shader_stages,
		.pImmutableSamplers = immutable_sampler
	});

	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::BuildDescSetLayout()
{
	// Grab the current configuration
	VulkanPipelineConfiguration& config = m_configurations.back();

	// Attach bindings
	config.ds_layout_info.bindingCount = static_cast<uint32_t>(config.bindings.size());
	config.ds_layout_info.pBindings = config.bindings.data();

	// Create descriptor set
	if (vkCreateDescriptorSetLayout(m_logical_device->handle, &config.ds_layout_info, nullptr, &config.ds_layouts.back()) != VK_SUCCESS)
		AURION_ERROR("[Vulkan Pipeline Builder] Failed to create descriptor set layout!");

	// Clear bindings
	config.bindings.clear();

	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::AddPushConstantRange(const VkShaderStageFlags& shader_stages, const uint32_t& offset, const uint32_t& size)
{
	// Grab the current configuration
	VulkanPipelineConfiguration& config = m_configurations.back();

	config.push_constants.emplace_back(VkPushConstantRange{
			.stageFlags = shader_stages,
			.offset = offset,
			.size = size
	});

	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::BuildPipelineLayout()
{
	// Grab the current configuration
	VulkanPipelineConfiguration& config = m_configurations.back();

	// Grab the target layout pointer
	VkPipelineLayout* out_layout = nullptr;
	switch (config.type)
	{
		case VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO:
		{
			out_layout = &config.compute_info.layout;
			break;
		}
		case VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO:
		{
			out_layout = &config.graphics_info.layout;
			break;
		}
		case VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR:
		{
			out_layout = &config.raytracing_info.layout;
			break;
		}
		default:
			return *this;
	}

	// Attach descriptor set layouts and push constants
	config.layout_info.pushConstantRangeCount = static_cast<uint32_t>(config.push_constants.size());
	config.layout_info.pPushConstantRanges = config.push_constants.data();
	config.layout_info.setLayoutCount = static_cast<uint32_t>(config.ds_layouts.size());
	config.layout_info.pSetLayouts = config.ds_layouts.data();

	// Build current pipeline layout configuration
	if (vkCreatePipelineLayout(m_logical_device->handle, &config.layout_info, nullptr, out_layout) != VK_SUCCESS)
		AURION_ERROR("[Vulkan Pipeline Builder] Failed to create pipeline layout!");

	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::UseDynamicRendering()
{
	// Grab the current configuration
	VulkanPipelineConfiguration& config = m_configurations.back();

	config.use_dynamic_rendering = true;

	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::SetDynamicViewMask(const uint32_t& view_mask)
{
	// Grab the current configuration
	VulkanPipelineConfiguration& config = m_configurations.back();

	config.dynamic_render_info.viewMask = view_mask;

	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::AddDynamicColorAttachmentFormat(const VkFormat& format)
{
	// Grab the current configuration
	VulkanPipelineConfiguration& config = m_configurations.back();

	config.dynamic_color_attachment_formats.emplace_back(format);

	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::SetDynamicDepthAttachmentFormat(const VkFormat& format)
{
	// Grab the current configuration
	VulkanPipelineConfiguration& config = m_configurations.back();

	config.dynamic_render_info.depthAttachmentFormat = format;

	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::SetDynamicStencilAttachmentFormat(const VkFormat& format)
{
	// Grab the current configuration
	VulkanPipelineConfiguration& config = m_configurations.back();

	config.dynamic_render_info.stencilAttachmentFormat = format;

	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::ConfigureRenderPass(const VkRenderPassCreateFlags& create_flags)
{
	// Grab the current configuration
	VulkanPipelineConfiguration& config = m_configurations.back();

	// Update create info for render pass
	config.render_pass.create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	config.render_pass.create_info.flags = create_flags;

	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::AddRenderPassAtachment(const VkAttachmentDescription& description)
{
	// Grab the current configuration
	VulkanPipelineConfiguration& config = m_configurations.back();

	// Attach description
	config.render_pass.attachments.emplace_back(description);

	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::ConfigureNewSubPass(const VkSubpassDescriptionFlags& create_flags, const VkPipelineBindPoint& bind_point)
{
	// Grab the current configuration
	VulkanPipelineConfiguration& config = m_configurations.back();

	// Add New subpass
	config.render_pass.subpasses.emplace_back(VulkanRenderPassConfiguration::Subpass{
		.description = VkSubpassDescription{ .flags = create_flags, .pipelineBindPoint = bind_point }
	});

	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::AddSubpassInputAttachment(const uint32_t& attachment, const VkImageLayout& layout)
{
	// Grab the current configuration
	VulkanPipelineConfiguration& config = m_configurations.back();

	// Grab subpass information
	VulkanRenderPassConfiguration::Subpass& subpass = config.render_pass.subpasses.back();
	
	// Add Attachment
	subpass.input_attachments.emplace_back(VkAttachmentReference{ attachment, layout });

	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::AddSubpassColorAttachment(const uint32_t& attachment, const VkImageLayout& layout)
{
	// Grab the current configuration
	VulkanPipelineConfiguration& config = m_configurations.back();

	// Grab subpass information
	VulkanRenderPassConfiguration::Subpass& subpass = config.render_pass.subpasses.back();

	// Add Attachment
	subpass.color_attachments.emplace_back(VkAttachmentReference{ attachment, layout });

	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::AddSubpassResolveAttachment(const uint32_t& attachment, const VkImageLayout& layout)
{
	// Grab the current configuration
	VulkanPipelineConfiguration& config = m_configurations.back();

	// Grab subpass information
	VulkanRenderPassConfiguration::Subpass& subpass = config.render_pass.subpasses.back();

	// Add Attachment
	subpass.resolve_attachments.emplace_back(VkAttachmentReference{ attachment, layout });

	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::AddSubpassPreserveAttachment(const uint32_t& attachment)
{
	// Grab the current configuration
	VulkanPipelineConfiguration& config = m_configurations.back();

	// Grab subpass information
	VulkanRenderPassConfiguration::Subpass& subpass = config.render_pass.subpasses.back();

	// Add Attachment
	subpass.preserve_attachments.emplace_back(attachment);

	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::BuildSubpass()
{
	// Grab the current configuration
	VulkanPipelineConfiguration& config = m_configurations.back();

	// Grab subpass information
	VulkanRenderPassConfiguration::Subpass& subpass = config.render_pass.subpasses.back();
	VkSubpassDescription& desc = subpass.description;

	// Attach subpass attachments
	desc.inputAttachmentCount = static_cast<uint32_t>(subpass.input_attachments.size());
	desc.colorAttachmentCount = static_cast<uint32_t>(subpass.color_attachments.size());
	desc.preserveAttachmentCount = static_cast<uint32_t>(subpass.preserve_attachments.size());
	desc.pInputAttachments = subpass.input_attachments.data();
	desc.pColorAttachments = subpass.color_attachments.data();
	desc.pResolveAttachments = subpass.resolve_attachments.data();
	desc.pPreserveAttachments = subpass.preserve_attachments.data();

	// Copy subpass information to render pass
	config.render_pass.subpass_descriptions.emplace_back(desc);

	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::AddSubpassDependency(const VkSubpassDependency& dependency)
{
	// Grab the current configuration
	VulkanPipelineConfiguration& config = m_configurations.back();

	// Add dependency
	config.render_pass.subpass_dependencies.emplace_back(dependency);

	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::BuildRenderPass()
{
	// Grab the current configuration
	VulkanPipelineConfiguration& config = m_configurations.back();

	// Copy over each subpass description
	for (size_t i = 0; i < config.render_pass.subpasses.size(); i++)
		config.render_pass.subpass_descriptions.emplace_back(config.render_pass.subpasses[i].description);

	// Attach render pass attachments and subpass info
	config.render_pass.create_info.attachmentCount = static_cast<uint32_t>(config.render_pass.attachments.size());
	config.render_pass.create_info.subpassCount = static_cast<uint32_t>(config.render_pass.subpass_descriptions.size());
	config.render_pass.create_info.dependencyCount = static_cast<uint32_t>(config.render_pass.subpass_dependencies.size());
	config.render_pass.create_info.pAttachments = config.render_pass.attachments.data();
	config.render_pass.create_info.pSubpasses = config.render_pass.subpass_descriptions.data();
	config.render_pass.create_info.pDependencies = config.render_pass.subpass_dependencies.data();
	
	// Ensure graphics create struct references subpass 0
	config.graphics_info.subpass = 0;

	// Build render pass
	if (vkCreateRenderPass(m_logical_device->handle, &config.render_pass.create_info, nullptr, &config.graphics_info.renderPass) != VK_SUCCESS)
		AURION_CRITICAL("[Vulkan Pipeline Builder] Failed to create render pass!");

	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::ConfigureVertexInputState(const VkPipelineVertexInputStateCreateFlags& create_flags)
{
	// Grab the current configuration
	VulkanPipelineConfiguration& config = m_configurations.back();

	// Setup vertex input state struct
	config.vertex_input_state = VkPipelineVertexInputStateCreateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.flags = create_flags
	};

	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::AddVertexBindingDescription(const uint32_t& binding, const uint32_t& stride, const VkVertexInputRate& input_rate)
{
	// Grab the current configuration
	VulkanPipelineConfiguration& config = m_configurations.back();

	// Attach binding description
	config.vertex_bindings.emplace_back(VkVertexInputBindingDescription{
		.binding = binding,
		.stride = stride,
		.inputRate = input_rate
	});

	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::AddVertexAttributeDescription(const uint32_t& location, const uint32_t& binding, const VkFormat& format, const uint32_t& offset)
{
	// Grab the current configuration
	VulkanPipelineConfiguration& config = m_configurations.back();

	// Attach attribute description
	config.vertex_attributes.emplace_back(VkVertexInputAttributeDescription{
		.location = location,
		.binding = binding,
		.format = format,
		.offset = offset
	});

	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::BuildVertexInputState()
{
	// Grab the current configuration
	VulkanPipelineConfiguration& config = m_configurations.back();

	// Attach binding and attribute descriptions
	config.vertex_input_state.vertexBindingDescriptionCount = static_cast<uint32_t>(config.vertex_bindings.size());
	config.vertex_input_state.vertexAttributeDescriptionCount = static_cast<uint32_t>(config.vertex_attributes.size());
	config.vertex_input_state.pVertexBindingDescriptions = config.vertex_bindings.data();
	config.vertex_input_state.pVertexAttributeDescriptions = config.vertex_attributes.data();

	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::ConfigureInputAssemblyState(const VkPipelineInputAssemblyStateCreateFlags& create_flags)
{
	// Grab the current configuration
	VulkanPipelineConfiguration& config = m_configurations.back();

	// Setup input assembly state struct
	config.input_assembly_state = VkPipelineInputAssemblyStateCreateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.flags = create_flags
	};

	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::SetPrimitiveTopology(const VkPrimitiveTopology& topology)
{
	// Grab the current configuration
	VulkanPipelineConfiguration& config = m_configurations.back();

	// Update state
	config.input_assembly_state.topology = topology;

	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::SetPrimitiveRestartEnable(const VkBool32& prim_restart_enable)
{
	// Grab the current configuration
	VulkanPipelineConfiguration& config = m_configurations.back();

	// Update state
	config.input_assembly_state.primitiveRestartEnable = prim_restart_enable;

	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::ConfigureTessellationState(const VkPipelineTessellationStateCreateFlags& create_flags)
{
	// Grab the current configuration
	VulkanPipelineConfiguration& config = m_configurations.back();

	// Setup tessellation state struct
	config.tessellation_state = VkPipelineTessellationStateCreateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
		.flags = create_flags
	};

	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::SetPatchControlPointCount(const uint32_t& count)
{
	// Grab the current configuration
	VulkanPipelineConfiguration& config = m_configurations.back();

	// Update state
	config.tessellation_state.patchControlPoints = count;

	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::ConfigureViewportState(const VkPipelineViewportStateCreateFlags& create_flags)
{
	// Grab the current configuration
	VulkanPipelineConfiguration& config = m_configurations.back();

	// Setup viewport state struct
	config.viewport_state = VkPipelineViewportStateCreateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.flags = create_flags
	};

	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::AddViewport(const VkViewport& viewport)
{
	// Grab the current configuration
	VulkanPipelineConfiguration& config = m_configurations.back();

	// Update state
	config.viewports.emplace_back(viewport);

	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::AddScissor(const VkRect2D& scissor)
{
	// Grab the current configuration
	VulkanPipelineConfiguration& config = m_configurations.back();

	// Update state
	config.scissors.emplace_back(scissor);

	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::BuildViewportState()
{
	// Grab the current configuration
	VulkanPipelineConfiguration& config = m_configurations.back();

	// Attach viewport and scissors
	config.viewport_state.viewportCount = static_cast<uint32_t>(config.viewports.size());
	config.viewport_state.scissorCount = static_cast<uint32_t>(config.scissors.size());
	config.viewport_state.pViewports = config.viewports.data();
	config.viewport_state.pScissors = config.scissors.data();

	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::ConfigureRasterizationState(const VkPipelineRasterizationStateCreateFlags& create_flags)
{
	// Grab the current configuration
	VulkanPipelineConfiguration& config = m_configurations.back();

	// Setup rasterization state struct
	config.rasterization_state = VkPipelineRasterizationStateCreateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.flags = create_flags
	};

	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::SetRasterizerDiscardEnabled(const VkBool32& rast_discard_enable)
{
	// Grab the current configuration
	VulkanPipelineConfiguration& config = m_configurations.back();

	// Update State
	config.rasterization_state.rasterizerDiscardEnable = rast_discard_enable;

	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::SetPolygonMode(const VkPolygonMode& mode)
{
	// Grab the current configuration
	VulkanPipelineConfiguration& config = m_configurations.back();

	// Update State
	config.rasterization_state.polygonMode = mode;

	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::SetCullMode(const VkCullModeFlags& mode)
{
	// Grab the current configuration
	VulkanPipelineConfiguration& config = m_configurations.back();

	// Update State
	config.rasterization_state.cullMode = mode;

	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::SetFrontFace(const VkFrontFace& front_face)
{
	// Grab the current configuration
	VulkanPipelineConfiguration& config = m_configurations.back();

	// Update State
	config.rasterization_state.frontFace = front_face;

	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::SetDepthClampEnabled(const VkBool32& depth_clamp_enable)
{
	// Grab the current configuration
	VulkanPipelineConfiguration& config = m_configurations.back();

	// Update State
	config.rasterization_state.depthClampEnable = depth_clamp_enable;

	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::SetDepthBiasEnabled(const VkBool32& depth_bias_enable)
{
	// Grab the current configuration
	VulkanPipelineConfiguration& config = m_configurations.back();

	// Update State
	config.rasterization_state.depthBiasEnable = depth_bias_enable;

	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::SetDepthBiasConstantFactor(const float& constant_factor)
{
	// Grab the current configuration
	VulkanPipelineConfiguration& config = m_configurations.back();

	// Update State
	config.rasterization_state.depthBiasConstantFactor = constant_factor;

	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::SetDepthBiasClamp(const float& bias_clamp)
{
	// Grab the current configuration
	VulkanPipelineConfiguration& config = m_configurations.back();

	// Update State
	config.rasterization_state.depthBiasClamp = bias_clamp;

	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::SetDepthBiasSlopeFactor(const float& slope_factor)
{
	// Grab the current configuration
	VulkanPipelineConfiguration& config = m_configurations.back();

	// Update State
	config.rasterization_state.depthBiasSlopeFactor = slope_factor;

	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::SetLineWidth(const float& line_width)
{
	// Grab the current configuration
	VulkanPipelineConfiguration& config = m_configurations.back();

	// Update State
	config.rasterization_state.lineWidth = line_width;

	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::ConfigureMultisampleState(const VkPipelineMultisampleStateCreateFlags& create_flags)
{
	// Grab the current configuration
	VulkanPipelineConfiguration& config = m_configurations.back();

	// Setup multisample state struct
	config.multisample_state = VkPipelineMultisampleStateCreateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.flags = create_flags
	};

	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::SetRasterizationSamples(const VkSampleCountFlagBits& sample_count)
{
	// Grab the current configuration
	VulkanPipelineConfiguration& config = m_configurations.back();

	// Update State
	config.multisample_state.rasterizationSamples = sample_count;

	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::SetSampleShadingEnabled(const VkBool32& enable)
{
	// Grab the current configuration
	VulkanPipelineConfiguration& config = m_configurations.back();

	// Update State
	config.multisample_state.sampleShadingEnable = enable;

	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::SetMinSampleShading(const float& min_sample_shading)
{
	// Grab the current configuration
	VulkanPipelineConfiguration& config = m_configurations.back();

	// Update State
	config.multisample_state.minSampleShading = min_sample_shading;

	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::AddSampleMask(const VkSampleMask& mask)
{
	// Grab the current configuration
	VulkanPipelineConfiguration& config = m_configurations.back();

	// Update State
	config.multisample_masks.emplace_back(mask);

	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::SetAlphaToCoverageEnabled(const VkBool32& enable)
{
	// Grab the current configuration
	VulkanPipelineConfiguration& config = m_configurations.back();

	// Update State
	config.multisample_state.alphaToCoverageEnable = enable;

	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::SetAlphaToOneEnabled(const VkBool32& enable)
{
	// Grab the current configuration
	VulkanPipelineConfiguration& config = m_configurations.back();

	// Update State
	config.multisample_state.alphaToOneEnable = enable;

	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::BuildMultisampleState()
{
	// Grab the current configuration
	VulkanPipelineConfiguration& config = m_configurations.back();

	// Attach sample masks
	config.multisample_state.pSampleMask = config.multisample_masks.data();

	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::ConfigureDepthStencilState(const VkPipelineDepthStencilStateCreateFlags& create_flags)
{
	// Grab the current configuration
	VulkanPipelineConfiguration& config = m_configurations.back();

	// Setup depth stencil state struct
	config.depth_stencil_state = VkPipelineDepthStencilStateCreateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.flags = create_flags
	};

	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::SetDepthTestEnabled(const VkBool32& enable)
{
	// Grab the current configuration
	VulkanPipelineConfiguration& config = m_configurations.back();

	// Update State
	config.depth_stencil_state.depthTestEnable = enable;

	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::SetDepthWriteEnabled(const VkBool32& enable)
{
	// Grab the current configuration
	VulkanPipelineConfiguration& config = m_configurations.back();

	// Update State
	config.depth_stencil_state.depthWriteEnable = enable;

	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::SetDepthCompareOp(const VkCompareOp& op)
{
	// Grab the current configuration
	VulkanPipelineConfiguration& config = m_configurations.back();

	// Update State
	config.depth_stencil_state.depthCompareOp = op;

	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::SetDepthBoundsTestEnabled(const VkBool32& enable)
{
	// Grab the current configuration
	VulkanPipelineConfiguration& config = m_configurations.back();

	// Update State
	config.depth_stencil_state.depthBoundsTestEnable = enable;

	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::SetStencilTestEnabled(const VkBool32& enable)
{
	// Grab the current configuration
	VulkanPipelineConfiguration& config = m_configurations.back();

	// Update State
	config.depth_stencil_state.stencilTestEnable = enable;

	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::SetFrontStencilOpState(const VkStencilOpState& state)
{
	// Grab the current configuration
	VulkanPipelineConfiguration& config = m_configurations.back();

	// Update State
	config.depth_stencil_state.front = state;

	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::SetBackStencilOpState(const VkStencilOpState& state)
{
	// Grab the current configuration
	VulkanPipelineConfiguration& config = m_configurations.back();

	// Update State
	config.depth_stencil_state.back = state;

	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::SetMinDepthBounds(const float& min_bounds)
{
	// Grab the current configuration
	VulkanPipelineConfiguration& config = m_configurations.back();

	// Update State
	config.depth_stencil_state.minDepthBounds = min_bounds;

	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::SetMaxDepthBounds(const float& max_bounds)
{
	// Grab the current configuration
	VulkanPipelineConfiguration& config = m_configurations.back();

	// Update State
	config.depth_stencil_state.maxDepthBounds = max_bounds;

	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::ConfigureColorBlendState(const VkPipelineColorBlendStateCreateFlags& create_flags)
{
	// Grab the current configuration
	VulkanPipelineConfiguration& config = m_configurations.back();

	// Setup dynamic state struct
	config.color_blend_state = VkPipelineColorBlendStateCreateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.flags = create_flags
	};

	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::SetLogicOpEnabled(const VkBool32& enable)
{
	// Grab the current configuration
	VulkanPipelineConfiguration& config = m_configurations.back();

	// Update State
	config.color_blend_state.logicOpEnable = enable;

	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::SetLogicOp(const VkLogicOp& op)
{
	// Grab the current configuration
	VulkanPipelineConfiguration& config = m_configurations.back();

	// Update State
	config.color_blend_state.logicOp = op;

	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::SetBlendConstants(const float& r, const float& g, const float& b, const float& a)
{
	// Grab the current configuration
	VulkanPipelineConfiguration& config = m_configurations.back();

	// Update State
	config.color_blend_state.blendConstants[0] = r;
	config.color_blend_state.blendConstants[1] = g;
	config.color_blend_state.blendConstants[2] = b;
	config.color_blend_state.blendConstants[3] = a;

	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::AddColorAttachment()
{
	// Grab the current configuration
	VulkanPipelineConfiguration& config = m_configurations.back();

	// Add attachment
	config.color_blend_attachments.emplace_back(VkPipelineColorBlendAttachmentState{});

	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::SetBlendEnabled(const VkBool32& enable)
{
	// Grab the current configuration
	VulkanPipelineConfiguration& config = m_configurations.back();

	// Grab current attachment
	VkPipelineColorBlendAttachmentState& state = config.color_blend_attachments.back();

	// Update state
	state.blendEnable = enable;

	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::SetSrcColorBlendFactor(const VkBlendFactor& blend_factor)
{
	// Grab the current configuration
	VulkanPipelineConfiguration& config = m_configurations.back();

	// Grab current attachment
	VkPipelineColorBlendAttachmentState& state = config.color_blend_attachments.back();

	// Update state
	state.srcColorBlendFactor = blend_factor;

	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::SetDstColorBlendFactor(const VkBlendFactor& blend_factor)
{
	// Grab the current configuration
	VulkanPipelineConfiguration& config = m_configurations.back();

	// Grab current attachment
	VkPipelineColorBlendAttachmentState& state = config.color_blend_attachments.back();

	// Update state
	state.dstColorBlendFactor = blend_factor;

	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::SetColorBlendOp(const VkBlendOp& blend_op)
{
	// Grab the current configuration
	VulkanPipelineConfiguration& config = m_configurations.back();

	// Grab current attachment
	VkPipelineColorBlendAttachmentState& state = config.color_blend_attachments.back();

	// Update state
	state.colorBlendOp = blend_op;

	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::SetSrcAlphaBlendFactor(const VkBlendFactor& blend_factor)
{
	// Grab the current configuration
	VulkanPipelineConfiguration& config = m_configurations.back();

	// Grab current attachment
	VkPipelineColorBlendAttachmentState& state = config.color_blend_attachments.back();

	// Update state
	state.srcAlphaBlendFactor = blend_factor;

	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::SetDstAlphaBlendFactor(const VkBlendFactor& blend_factor)
{
	// Grab the current configuration
	VulkanPipelineConfiguration& config = m_configurations.back();

	// Grab current attachment
	VkPipelineColorBlendAttachmentState& state = config.color_blend_attachments.back();

	// Update state
	state.dstAlphaBlendFactor = blend_factor;

	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::SetAlphaBlendOp(const VkBlendOp& blend_op)
{
	// Grab the current configuration
	VulkanPipelineConfiguration& config = m_configurations.back();

	// Grab current attachment
	VkPipelineColorBlendAttachmentState& state = config.color_blend_attachments.back();

	// Update state
	state.alphaBlendOp = blend_op;

	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::SetColorWriteMask(const VkColorComponentFlags& write_mask)
{
	// Grab the current configuration
	VulkanPipelineConfiguration& config = m_configurations.back();

	// Grab current attachment
	VkPipelineColorBlendAttachmentState& state = config.color_blend_attachments.back();

	// Update state
	state.colorWriteMask = write_mask;

	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::BuildColorBlendState()
{
	// Grab the current configuration
	VulkanPipelineConfiguration& config = m_configurations.back();

	// Attach color blend attachments
	config.color_blend_state.attachmentCount = static_cast<uint32_t>(config.color_blend_attachments.size());
	config.color_blend_state.pAttachments = config.color_blend_attachments.data();

	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::ConfigureDynamicState(const VkPipelineDynamicStateCreateFlags& create_flags)
{
	// Grab the current configuration
	VulkanPipelineConfiguration& config = m_configurations.back();

	// Setup dynamic state struct
	config.dynamic_state = VkPipelineDynamicStateCreateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.flags = create_flags
	};

	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::AddDynamicState(const VkDynamicState& state)
{
	// Grab the current configuration
	VulkanPipelineConfiguration& config = m_configurations.back();

	config.dynamic_states.emplace_back(state);

	return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::BuildDynamicState()
{
	// Grab the current configuration
	VulkanPipelineConfiguration& config = m_configurations.back();

	config.dynamic_state.dynamicStateCount = static_cast<uint32_t>(config.dynamic_states.size());
	config.dynamic_state.pDynamicStates = config.dynamic_states.data();

	return *this;
}

shaderc_shader_kind VulkanPipelineBuilder::VkShaderToShaderc(const VkShaderStageFlags& flags)
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


