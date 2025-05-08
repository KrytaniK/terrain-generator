#include <macros/AurionLog.h>

#include <cstdint>
#include <vector>
#include <stdexcept>

#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

import Vulkan;
import Debug;

VulkanRenderer::VulkanRenderer()
	: m_max_in_flight_frames(1)
{
	
}

VulkanRenderer::~VulkanRenderer()
{
	// No need to destroy vulkan resources if vulkan was never initialized
	if (m_logical_device.vk_instance == VK_NULL_HANDLE)
		return;

	// Wait for GPU to finish work
	vkDeviceWaitIdle(m_logical_device.handle);

	// Cleanup any pipeline resources
	for (size_t i = 0; i < m_pipelines.size(); i++)
	{
		VulkanPipeline& pipeline = m_pipelines[i];

		// Clean up render pass
		vkDestroyRenderPass(m_logical_device.handle, pipeline.render_pass, nullptr);

		// Clean up pipeline layout
		vkDestroyPipelineLayout(m_logical_device.handle, pipeline.layout, nullptr);

		// Clean up pipeline handle
		vkDestroyPipeline(m_logical_device.handle, pipeline.handle, nullptr);
	}

	// Ensure all window resources have been cleaned up BEFORE the logical
	//	device is destroyed
	m_contexts.clear();

	// Destroy VMA allocator
	vmaDestroyAllocator(m_logical_device.allocator);

	// Destroy logical device
	vkDestroyDevice(m_logical_device.handle, nullptr);
}

void VulkanRenderer::Initialize()
{
	m_logical_device = VulkanDevice::Create(m_vk_instance, *m_config);
}

void VulkanRenderer::Render()
{
	for (size_t i = 0; i < m_contexts.size(); i++)
		m_contexts[i].RenderFrame();
}

VulkanContext* VulkanRenderer::CreateContext(const Aurion::WindowHandle& handle)
{
	// Ensure a context doesn't exist for this window
	for (size_t i = 0; i < m_contexts.size(); i++)
	{
		if (m_contexts[i].GetContextID() == handle.id)
		{
			AURION_ERROR("[Vulkan Renderer] Failed to create context for window with id [%d]: Context already exists!", handle.id);
			return nullptr;
		}
	}

	m_contexts.emplace_back();

	// Initialized the context
	VulkanContext& context = m_contexts.back();
	context.SetLogicalDevice(&m_logical_device);
	context.SetMaxInFlightFrames(m_max_in_flight_frames);
	context.SetWindow(handle);
	context.Initialize();

	return &context;
}

VulkanContext* VulkanRenderer::GetContext(const uint64_t& id)
{
	// Search for the context by window id
	for (size_t i = 0; i < m_contexts.size(); i++)
		if (m_contexts[i].GetContextID() == id)
			return &m_contexts[i];

	return nullptr;
}

bool VulkanRenderer::RemoveContext(const uint64_t& id)
{
	// Search for the context by window id
	size_t remove_index = m_contexts.size();
	for (size_t i = 0; i < m_contexts.size(); i++)
		if (m_contexts[i].GetContextID() == id)
			remove_index = i;

	if (remove_index == m_contexts.size())
		return false;

	m_contexts.erase(m_contexts.begin() + remove_index);
	return true;
}

void VulkanRenderer::SetConfiguration(const VkInstance& vk_instance, const VulkanDeviceConfiguration* device_config, const uint32_t& max_in_flight_frames)
{
	if (m_logical_device.vk_instance != VK_NULL_HANDLE)
	{
		AURION_WARN("[Vulkan Renderer] Attempt to initialize after initialization!");
		return;
	}

	m_vk_instance = vk_instance;
	m_config = device_config;
	m_max_in_flight_frames = max_in_flight_frames;
}

std::vector<VulkanPipeline>& VulkanRenderer::GetVkPipelineBuffer()
{
	return m_pipelines;
}

VulkanDevice* VulkanRenderer::GetLogicalDevice()
{
	return &m_logical_device;
}

const uint32_t& VulkanRenderer::GetMaxFramesInFlight()
{
	return m_max_in_flight_frames;
}
