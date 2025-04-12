#include <macros/AurionLog.h>

#include <string>
#include <vector>
#include <cstdint>
#include <thread>

#include <shaderc/shaderc.hpp>
#include <spirv_cross/spirv_cross.hpp>
#include <vulkan/vulkan.h>

import Vulkan;

VulkanPipelineFactory::VulkanPipelineFactory()
{

}

VulkanPipelineFactory::~VulkanPipelineFactory()
{

}

void VulkanPipelineFactory::Initialize(VulkanDevice* device, std::vector<VulkanPipeline>& pipeline_buffer)
{
	this->m_logical_device = device;
	this->m_pipeline_buffer = &pipeline_buffer;
}

void VulkanPipelineFactory::Cleanup()
{
	this->m_strategies.clear();
}

std::span<VulkanPipeline> VulkanPipelineFactory::Build()
{
	// Calculate pipeline span extents before building pipelines
	size_t span_begin = m_pipeline_buffer->size();
	size_t span_end = span_begin + m_strategies.size();

	// C++ doesn't allow arrays of references, so full copies must be made
	std::vector<VkGraphicsPipelineCreateInfo> graphics_infos;
	// TODO: Compute
	// TODO: Raytracing

	// For each strategy, grab the relevant build information and
	//	grab a reference to the pipeline handle.
	for (size_t i = 0; i < m_strategies.size(); i++)
	{
		VulkanPipelineInfo create_info = m_strategies[i]->Build();
		VulkanPipeline& pipeline = m_pipeline_buffer->emplace_back();

		pipeline.type = m_strategies[i]->GetType();
		switch (pipeline.type)
		{
			case VULKAN_PIPELINE_GRAPHICS:
			{
				// Attach layout and render pass handles
				pipeline.layout = create_info.graphics.layout;
				pipeline.render_pass = create_info.graphics.renderPass;

				// Copy over graphics pipeline info
				graphics_infos.emplace_back(create_info.graphics);
				break;
			}
			case VULKAN_PIPELINE_COMPUTE:
			{
				pipeline.layout = create_info.compute.layout;
				// TODO: Compute
				break;
			}
			case VULKAN_PIPELINE_RAY_TRACING:
			{
				pipeline.layout = create_info.raytracing.layout;
				// TODO: Raytracing
				break;
			}
			default: continue;
		}
	}

	// Batch Build all pipelines
	std::vector<VkPipeline> graphics_pipelines(graphics_infos.size());
	if (VK_SUCCESS != vkCreateGraphicsPipelines(m_logical_device->handle, VK_NULL_HANDLE, static_cast<uint32_t>(graphics_pipelines.size()), graphics_infos.data(), nullptr, graphics_pipelines.data()))
		AURION_ERROR("[Vulkan Pipeline Factory] Failed to create graphics pipelines!");

	// Copy over pipeline graphics handles
	size_t handle_index = 0;
	for (size_t i = span_begin; i < span_end; i++)
	{
		VulkanPipeline& pipeline = m_pipeline_buffer->at(i);
		if (pipeline.type == VULKAN_PIPELINE_GRAPHICS)
			pipeline.handle = graphics_pipelines[handle_index++];
	}

	// Copy over pipeline compute handles (TODO)
	handle_index = 0;
	for (size_t i = span_begin; i < span_end; i++)
	{
		VulkanPipeline& pipeline = m_pipeline_buffer->at(i);
		if (pipeline.type == VULKAN_PIPELINE_COMPUTE)
			continue; // TODO: Compute
	}

	// Copy over pipeline raytracing handles (TODO)
	handle_index = 0;
	for (size_t i = span_begin; i < span_end; i++)
	{
		VulkanPipeline& pipeline = m_pipeline_buffer->at(i);
		if (pipeline.type == VULKAN_PIPELINE_RAY_TRACING)
			continue; // TODO: Raytracing
	}

	// Return span of pipelines built
	return std::span<VulkanPipeline>(m_pipeline_buffer->begin() + span_begin, m_pipeline_buffer->begin() + span_end);
}