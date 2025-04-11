#include <macros/AurionLog.h>

#include <vulkan/vulkan.h>

import Vulkan;

VulkanGraphicsPipeline::VulkanGraphicsPipeline()
	: m_logical_device(nullptr)
{
	
}

VulkanGraphicsPipeline::~VulkanGraphicsPipeline()
{
	// Cleanup shader modules
	for (size_t i = 0; i < m_shader_stages.size(); i++)
		vkDestroyShaderModule(m_logical_device->handle, m_shader_stages[i].module, nullptr);
}

VulkanPipelineType VulkanGraphicsPipeline::GetType()
{
	return VULKAN_PIPELINE_GRAPHICS;
}

void VulkanGraphicsPipeline::Initialize(VulkanDevice* device)
{
	m_logical_device = device;

	m_create_info.graphics = VkGraphicsPipelineCreateInfo{
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO
	};

	m_layout_info = VkPipelineLayoutCreateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO
	};

	m_state = ConfigurationState{};
}

VulkanPipelineInfo& VulkanGraphicsPipeline::Build()
{
	// Attach all structures
	// -------------------------------

	// Dynamic Rendering Information
	if (m_state.dynamic_rendering.enabled)
	{
		m_state.dynamic_rendering.create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
		m_state.dynamic_rendering.create_info.colorAttachmentCount = static_cast<uint32_t>(m_state.dynamic_rendering.color_attachment_formats.size());
		m_state.dynamic_rendering.create_info.pColorAttachmentFormats = m_state.dynamic_rendering.color_attachment_formats.data();
		m_create_info.graphics.pNext = &m_state.dynamic_rendering.create_info;
	}

	// Render Pass creation (Only if not using dynamic rendering)
	if (m_state.render_pass.enabled && !m_state.dynamic_rendering.enabled)
	{
		m_state.render_pass.create_info.attachmentCount = static_cast<uint32_t>(m_state.render_pass.attachments.size());
		m_state.render_pass.create_info.dependencyCount = static_cast<uint32_t>(m_state.render_pass.subpass_dependencies.size());
		m_state.render_pass.create_info.subpassCount = static_cast<uint32_t>(m_state.render_pass.subpasses.size());
		m_state.render_pass.create_info.pAttachments = m_state.render_pass.attachments.data();
		m_state.render_pass.create_info.pDependencies = m_state.render_pass.subpass_dependencies.data();
		m_state.render_pass.create_info.pSubpasses = m_state.render_pass.subpass_descriptions.data();

		if (vkCreateRenderPass(m_logical_device->handle, &m_state.render_pass.create_info, nullptr, &m_create_info.graphics.renderPass) != VK_SUCCESS)
		{
			AURION_ERROR("[Vulkan Graphics Pipeline] Failed to create render pass");
			return m_create_info;
		}
	}
	else
	{
		m_create_info.graphics.renderPass = VK_NULL_HANDLE;
	}

	// Vertex Input State
	m_state.vertex_input.vertexAttributeDescriptionCount = static_cast<uint32_t>(m_vertex_attributes.size());
	m_state.vertex_input.vertexBindingDescriptionCount = static_cast<uint32_t>(m_vertex_bindings.size());
	m_state.vertex_input.pVertexAttributeDescriptions = m_vertex_attributes.data();
	m_state.vertex_input.pVertexBindingDescriptions = m_vertex_bindings.data();
	m_create_info.graphics.pVertexInputState = &m_state.vertex_input;

	// Input Assembly State
	m_create_info.graphics.pInputAssemblyState = &m_state.input_assembly;

	// Tessellation State
	m_create_info.graphics.pTessellationState = &m_state.tessellation;

	// Viewport State
	m_state.viewport.viewportCount = static_cast<uint32_t>(m_viewports.size());
	m_state.viewport.scissorCount = static_cast<uint32_t>(m_scissors.size());
	m_state.viewport.pViewports = m_viewports.data();
	m_state.viewport.pScissors = m_scissors.data();
	m_create_info.graphics.pViewportState = &m_state.viewport;

	// Rasterization State
	m_create_info.graphics.pRasterizationState = &m_state.rasterization;

	// Multisampling State
	m_create_info.graphics.pMultisampleState = &m_state.multisampling;

	// Depth Stencil State
	m_create_info.graphics.pDepthStencilState = &m_state.depth_stencil;

	// Color Blend State
	m_state.color_blend.attachmentCount = static_cast<uint32_t>(m_color_blend_attachments.size());
	m_state.color_blend.pAttachments = m_color_blend_attachments.data();
	m_create_info.graphics.pColorBlendState = &m_state.color_blend;

	// Dynamic State
	m_state.dynamic_state.dynamicStateCount = static_cast<uint32_t>(m_dynamic_states.size());
	m_state.dynamic_state.pDynamicStates = m_dynamic_states.data();
	m_create_info.graphics.pDynamicState = &m_state.dynamic_state;

	// -------------------------------

	// Generate Pipeline Layout
	m_layout_info.pushConstantRangeCount = static_cast<uint32_t>(m_push_constants.size());
	m_layout_info.setLayoutCount = static_cast<uint32_t>(m_ds_layouts.size());
	m_layout_info.pPushConstantRanges = m_push_constants.data();
	m_layout_info.pSetLayouts = m_ds_layouts.data();
	
	if (vkCreatePipelineLayout(m_logical_device->handle, &m_layout_info, nullptr, &m_create_info.graphics.layout) != VK_SUCCESS)
	{
		AURION_ERROR("[Vulkan Graphics Pipeline] Failed to create pipeline layout");
		return m_create_info;
	}

	// Attach shader modules
	m_create_info.graphics.stageCount = static_cast<uint32_t>(m_shader_stages.size());
	m_create_info.graphics.pStages = m_shader_stages.data();

	// Return creation structure
	return m_create_info;
}

VulkanGraphicsPipeline& VulkanGraphicsPipeline::BindShader(const VkPipelineShaderStageCreateInfo& stage_info)
{
	m_shader_stages.emplace_back(stage_info);
	return *this;
}

VulkanGraphicsPipeline& VulkanGraphicsPipeline::ConfigurePipelineLayout(const VkPipelineLayoutCreateFlags& create_flags)
{
	m_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	m_layout_info.flags = create_flags;
	m_layout_info.setLayoutCount = 0;
	m_layout_info.pSetLayouts = nullptr;
	m_layout_info.pushConstantRangeCount = 0;
	m_layout_info.pPushConstantRanges = nullptr;

	return *this;
}

VulkanGraphicsPipeline& VulkanGraphicsPipeline::AddDescriptorSetLayout(const VkDescriptorSetLayout& layout)
{
	m_ds_layouts.emplace_back(layout);
	return *this;
}

VulkanGraphicsPipeline& VulkanGraphicsPipeline::AddPushConstantRange(const VkPushConstantRange& range)
{
	m_push_constants.emplace_back(range);
	return *this;
}

VulkanGraphicsPipeline& VulkanGraphicsPipeline::SetDynamicViewMask(const uint32_t& view_mask)
{
	m_state.dynamic_rendering.enabled = true;
	m_state.dynamic_rendering.create_info.viewMask = view_mask;
	return *this;
}

VulkanGraphicsPipeline& VulkanGraphicsPipeline::AddDynamicColorAttachmentFormat(const VkFormat& format)
{
	m_state.dynamic_rendering.enabled = true;
	m_state.dynamic_rendering.color_attachment_formats.emplace_back(format);
	return *this;
}

VulkanGraphicsPipeline& VulkanGraphicsPipeline::SetDynamicDepthAttachmentFormat(const VkFormat& format)
{
	m_state.dynamic_rendering.enabled = true;
	m_state.dynamic_rendering.create_info.depthAttachmentFormat = format;
	return *this;
}

VulkanGraphicsPipeline& VulkanGraphicsPipeline::SetDynamicStencilAttachmentFormat(const VkFormat& format)
{
	m_state.dynamic_rendering.enabled = true;
	m_state.dynamic_rendering.create_info.stencilAttachmentFormat = format;
	return *this;
}

VulkanGraphicsPipeline& VulkanGraphicsPipeline::SetDynamicRenderingEXT(void* extension)
{
	m_state.dynamic_rendering.enabled = true;
	m_state.dynamic_rendering.create_info.pNext = extension;
	return *this;
}

VulkanGraphicsPipeline& VulkanGraphicsPipeline::ConfigureRenderPass(const VkRenderPassCreateFlags& create_flags)
{
	m_state.render_pass.create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	m_state.render_pass.create_info.flags = create_flags;
	m_state.render_pass.enabled = true;
	return *this;
}

VulkanGraphicsPipeline& VulkanGraphicsPipeline::AddRenderPassAttachment(const VkAttachmentDescription& description)
{
	m_state.render_pass.attachments.emplace_back(description);
	return *this;
}

VulkanGraphicsPipeline& VulkanGraphicsPipeline::AddSubpassDependency(const VkSubpassDependency& dependency)
{
	m_state.render_pass.subpass_dependencies.emplace_back(dependency);
	return *this;
}

VulkanGraphicsPipeline& VulkanGraphicsPipeline::BeginSubpass(const VkSubpassDescriptionFlags& create_flags, const VkPipelineBindPoint& bind_point)
{
	m_state.render_pass.subpasses.emplace_back(RenderPass::Subpass{
		.description = VkSubpassDescription{
			.flags = create_flags,
			.pipelineBindPoint = bind_point
		}
	});
	return *this;
}

VulkanGraphicsPipeline& VulkanGraphicsPipeline::AddSubpassInputAttachment(const uint32_t& attachment, const VkImageLayout& layout)
{
	m_state.render_pass.subpasses.back().input_attachments.emplace_back(VkAttachmentReference{ attachment, layout });
	return *this;
}

VulkanGraphicsPipeline& VulkanGraphicsPipeline::AddSubpassColorAttachment(const uint32_t& attachment, const VkImageLayout& layout)
{
	m_state.render_pass.subpasses.back().color_attachments.emplace_back(VkAttachmentReference{ attachment, layout });
	return *this;
}

VulkanGraphicsPipeline& VulkanGraphicsPipeline::AddSubpassResolveAttachment(const uint32_t& attachment, const VkImageLayout& layout)
{
	m_state.render_pass.subpasses.back().resolve_attachments.emplace_back(VkAttachmentReference{ attachment, layout });
	return *this;
}

VulkanGraphicsPipeline& VulkanGraphicsPipeline::AddSubpassPreserveAttachment(const uint32_t& attachment)
{
	m_state.render_pass.subpasses.back().preserve_attachments.emplace_back(attachment);

	return *this;
}

VulkanGraphicsPipeline& VulkanGraphicsPipeline::EndSubpass()
{
	RenderPass::Subpass& subpass = m_state.render_pass.subpasses.back();
	VkSubpassDescription desc = subpass.description;

	// Attach subpass attachments
	desc.inputAttachmentCount = static_cast<uint32_t>(subpass.input_attachments.size());
	desc.colorAttachmentCount = static_cast<uint32_t>(subpass.color_attachments.size());
	desc.preserveAttachmentCount = static_cast<uint32_t>(subpass.preserve_attachments.size());
	desc.pInputAttachments = subpass.input_attachments.data();
	desc.pColorAttachments = subpass.color_attachments.data();
	desc.pResolveAttachments = subpass.resolve_attachments.data();
	desc.pPreserveAttachments = subpass.preserve_attachments.data();

	// Copy subpass information to render pass
	m_state.render_pass.subpass_descriptions.emplace_back(desc);

	return *this;
}

VulkanGraphicsPipeline& VulkanGraphicsPipeline::ConfigureVertexInputState(const VkPipelineVertexInputStateCreateFlags& create_flags)
{
	m_state.vertex_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	m_state.vertex_input.flags = create_flags;
	return *this;
}

VulkanGraphicsPipeline& VulkanGraphicsPipeline::AddVertexBindingDescription(const uint32_t& binding, const uint32_t& stride, const VkVertexInputRate& input_rate)
{
	m_vertex_bindings.emplace_back(VkVertexInputBindingDescription{
		.binding = binding,
		.stride = stride,
		.inputRate = input_rate
	});
	return *this;
}

VulkanGraphicsPipeline& VulkanGraphicsPipeline::AddVertexAttributeDescription(const uint32_t& location, const uint32_t& binding, const VkFormat& format, const uint32_t& offset)
{
	m_vertex_attributes.emplace_back(VkVertexInputAttributeDescription{
		.location = location,
		.binding = binding,
		.format = format,
		.offset = offset
	});
	return *this;
}

VulkanGraphicsPipeline& VulkanGraphicsPipeline::ConfigureInputAssemblyState(const VkPipelineInputAssemblyStateCreateFlags& create_flags)
{
	m_state.input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	m_state.input_assembly.flags = create_flags;
	return *this;
}

VulkanGraphicsPipeline& VulkanGraphicsPipeline::SetPrimitiveTopology(const VkPrimitiveTopology& topology)
{
	m_state.input_assembly.topology = topology;
	return *this;
}

VulkanGraphicsPipeline& VulkanGraphicsPipeline::SetPrimitiveRestartEnable(const VkBool32& prim_restart_enable)
{
	m_state.input_assembly.primitiveRestartEnable = prim_restart_enable;
	return *this;
}

VulkanGraphicsPipeline& VulkanGraphicsPipeline::ConfigureTessellationState(const VkPipelineTessellationStateCreateFlags& create_flags)
{
	m_state.tessellation.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
	m_state.tessellation.flags = create_flags;
	return *this;
}

VulkanGraphicsPipeline& VulkanGraphicsPipeline::SetPatchControlPointCount(const uint32_t& count)
{
	m_state.tessellation.patchControlPoints = count;
	return *this;
}

VulkanGraphicsPipeline& VulkanGraphicsPipeline::ConfigureViewportState(const VkPipelineViewportStateCreateFlags& create_flags)
{
	m_state.viewport.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	m_state.viewport.flags = create_flags;
	return *this;
}

VulkanGraphicsPipeline& VulkanGraphicsPipeline::AddViewport(const VkViewport& viewport)
{
	m_viewports.emplace_back(viewport);
	return *this;
}

VulkanGraphicsPipeline& VulkanGraphicsPipeline::AddScissor(const VkRect2D& scissor)
{
	m_scissors.emplace_back(scissor);
	return *this;
}

VulkanGraphicsPipeline& VulkanGraphicsPipeline::ConfigureRasterizationState(const VkPipelineRasterizationStateCreateFlags& create_flags)
{
	m_state.rasterization.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	m_state.rasterization.flags = create_flags;
	return *this;
}

VulkanGraphicsPipeline& VulkanGraphicsPipeline::SetRasterizerDiscardEnabled(const VkBool32& rast_discard_enable)
{
	m_state.rasterization.rasterizerDiscardEnable = rast_discard_enable;
	return *this;
}

VulkanGraphicsPipeline& VulkanGraphicsPipeline::SetPolygonMode(const VkPolygonMode& mode)
{
	m_state.rasterization.polygonMode = mode;
	return *this;
}

VulkanGraphicsPipeline& VulkanGraphicsPipeline::SetCullMode(const VkCullModeFlags& mode)
{
	m_state.rasterization.cullMode = mode;
	return *this;
}

VulkanGraphicsPipeline& VulkanGraphicsPipeline::SetFrontFace(const VkFrontFace& front_face)
{
	m_state.rasterization.frontFace = front_face;
	return *this;
}

VulkanGraphicsPipeline& VulkanGraphicsPipeline::SetDepthClampEnabled(const VkBool32& depth_clamp_enable)
{
	m_state.rasterization.depthClampEnable = depth_clamp_enable;
	return *this;
}

VulkanGraphicsPipeline& VulkanGraphicsPipeline::SetDepthBiasEnabled(const VkBool32& depth_bias_enable)
{
	m_state.rasterization.depthBiasEnable = depth_bias_enable;
	return *this;
}

VulkanGraphicsPipeline& VulkanGraphicsPipeline::SetDepthBiasConstantFactor(const float& constant_factor)
{
	m_state.rasterization.depthBiasConstantFactor = constant_factor;
	return *this;
}

VulkanGraphicsPipeline& VulkanGraphicsPipeline::SetDepthBiasClamp(const float& bias_clamp)
{
	m_state.rasterization.depthBiasClamp = bias_clamp;
	return *this;
}

VulkanGraphicsPipeline& VulkanGraphicsPipeline::SetDepthBiasSlopeFactor(const float& slope_factor)
{
	m_state.rasterization.depthBiasSlopeFactor = slope_factor;
	return *this;
}

VulkanGraphicsPipeline& VulkanGraphicsPipeline::SetLineWidth(const float& line_width)
{
	m_state.rasterization.lineWidth = line_width;
	return *this;
}

VulkanGraphicsPipeline& VulkanGraphicsPipeline::ConfigureMultisampleState(const VkPipelineMultisampleStateCreateFlags& create_flags)
{
	m_state.multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	m_state.multisampling.flags = create_flags;
	return *this;
}

VulkanGraphicsPipeline& VulkanGraphicsPipeline::SetRasterizationSamples(const VkSampleCountFlagBits& sample_count)
{
	m_state.multisampling.rasterizationSamples = sample_count;
	return *this;
}

VulkanGraphicsPipeline& VulkanGraphicsPipeline::SetSampleShadingEnabled(const VkBool32& enable)
{
	m_state.multisampling.sampleShadingEnable = enable;
	return *this;
}

VulkanGraphicsPipeline& VulkanGraphicsPipeline::SetMinSampleShading(const float& min_sample_shading)
{
	m_state.multisampling.minSampleShading = min_sample_shading;
	return *this;
}

VulkanGraphicsPipeline& VulkanGraphicsPipeline::SetSampleMask(const VkSampleMask& mask)
{
	m_state.multisampling.pSampleMask = &mask;
	return *this;
}

VulkanGraphicsPipeline& VulkanGraphicsPipeline::SetAlphaToCoverageEnabled(const VkBool32& enable)
{
	m_state.multisampling.alphaToCoverageEnable = enable;
	return *this;
}

VulkanGraphicsPipeline& VulkanGraphicsPipeline::SetAlphaToOneEnabled(const VkBool32& enable)
{
	m_state.multisampling.alphaToOneEnable = enable;
	return *this;
}

VulkanGraphicsPipeline& VulkanGraphicsPipeline::ConfigureDepthStencilState(const VkPipelineDepthStencilStateCreateFlags& create_flags)
{
	m_state.depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	m_state.depth_stencil.flags = create_flags;
	return *this;
}

VulkanGraphicsPipeline& VulkanGraphicsPipeline::SetDepthTestEnabled(const VkBool32& enable)
{
	m_state.depth_stencil.depthTestEnable = enable;
	return *this;
}

VulkanGraphicsPipeline& VulkanGraphicsPipeline::SetDepthWriteEnabled(const VkBool32& enable)
{
	m_state.depth_stencil.depthWriteEnable = enable;
	return *this;
}

VulkanGraphicsPipeline& VulkanGraphicsPipeline::SetDepthCompareOp(const VkCompareOp& op)
{
	m_state.depth_stencil.depthCompareOp = op;
	return *this;
}

VulkanGraphicsPipeline& VulkanGraphicsPipeline::SetDepthBoundsTestEnabled(const VkBool32& enable)
{
	m_state.depth_stencil.depthBoundsTestEnable = enable;
	return *this;
}

VulkanGraphicsPipeline& VulkanGraphicsPipeline::SetStencilTestEnabled(const VkBool32& enable)
{
	m_state.depth_stencil.stencilTestEnable = enable;
	return *this;
}

VulkanGraphicsPipeline& VulkanGraphicsPipeline::SetFrontStencilOpState(const VkStencilOpState& state)
{
	m_state.depth_stencil.front = state;
	return *this;
}

VulkanGraphicsPipeline& VulkanGraphicsPipeline::SetBackStencilOpState(const VkStencilOpState& state)
{
	m_state.depth_stencil.back = state;
	return *this;
}

VulkanGraphicsPipeline& VulkanGraphicsPipeline::SetMinDepthBounds(const float& min_bounds)
{
	m_state.depth_stencil.minDepthBounds = min_bounds;
	return *this;
}

VulkanGraphicsPipeline& VulkanGraphicsPipeline::SetMaxDepthBounds(const float& max_bounds)
{
	m_state.depth_stencil.maxDepthBounds = max_bounds;
	return *this;
}

VulkanGraphicsPipeline& VulkanGraphicsPipeline::ConfigureColorBlendState(const VkPipelineColorBlendStateCreateFlags& create_flags)
{
	m_state.color_blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	m_state.color_blend.flags = create_flags;
	return *this;
}

VulkanGraphicsPipeline& VulkanGraphicsPipeline::SetLogicOpEnabled(const VkBool32& enable)
{
	m_state.color_blend.logicOpEnable = enable;
	return *this;
}

VulkanGraphicsPipeline& VulkanGraphicsPipeline::SetLogicOp(const VkLogicOp& op)
{
	m_state.color_blend.logicOp = op;
	return *this;
}

VulkanGraphicsPipeline& VulkanGraphicsPipeline::SetBlendConstants(const float& r, const float& g, const float& b, const float& a)
{
	m_state.color_blend.blendConstants[0] = r;
	m_state.color_blend.blendConstants[1] = g;
	m_state.color_blend.blendConstants[2] = b;
	m_state.color_blend.blendConstants[3] = a;
	return *this;
}

VulkanGraphicsPipeline& VulkanGraphicsPipeline::AddColorAttachment()
{
	m_color_blend_attachments.emplace_back(VkPipelineColorBlendAttachmentState{});
	return *this;
}

VulkanGraphicsPipeline& VulkanGraphicsPipeline::SetBlendEnabled(const VkBool32& enable)
{
	m_color_blend_attachments.back().blendEnable = enable;
	return *this;
}

VulkanGraphicsPipeline& VulkanGraphicsPipeline::SetSrcColorBlendFactor(const VkBlendFactor& blend_factor)
{
	m_color_blend_attachments.back().srcColorBlendFactor = blend_factor;
	return *this;
}

VulkanGraphicsPipeline& VulkanGraphicsPipeline::SetDstColorBlendFactor(const VkBlendFactor& blend_factor)
{
	m_color_blend_attachments.back().dstColorBlendFactor = blend_factor;
	return *this;
}

VulkanGraphicsPipeline& VulkanGraphicsPipeline::SetColorBlendOp(const VkBlendOp& blend_op)
{
	m_color_blend_attachments.back().colorBlendOp = blend_op;
	return *this;
}

VulkanGraphicsPipeline& VulkanGraphicsPipeline::SetSrcAlphaBlendFactor(const VkBlendFactor& blend_factor)
{
	m_color_blend_attachments.back().srcAlphaBlendFactor = blend_factor;
	return *this;
}

VulkanGraphicsPipeline& VulkanGraphicsPipeline::SetDstAlphaBlendFactor(const VkBlendFactor& blend_factor)
{
	m_color_blend_attachments.back().dstAlphaBlendFactor = blend_factor;
	return *this;
}

VulkanGraphicsPipeline& VulkanGraphicsPipeline::SetAlphaBlendOp(const VkBlendOp& blend_op)
{
	m_color_blend_attachments.back().alphaBlendOp = blend_op;
	return *this;
}

VulkanGraphicsPipeline& VulkanGraphicsPipeline::SetColorWriteMask(const VkColorComponentFlags& write_mask)
{
	m_color_blend_attachments.back().colorWriteMask = write_mask;
	return *this;
}

VulkanGraphicsPipeline& VulkanGraphicsPipeline::ConfigureDynamicState(const VkPipelineDynamicStateCreateFlags& create_flags)
{
	m_state.dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	m_state.dynamic_state.flags = create_flags;
	return *this;
}

VulkanGraphicsPipeline& VulkanGraphicsPipeline::AddDynamicState(const VkDynamicState& state)
{
	m_dynamic_states.emplace_back(state);
	return *this;
}