#include <macros/AurionLog.h>

#include <cstdint>
#include <utility>
#include <unordered_map>

#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

import Vulkan;

VulkanRenderer::VulkanRenderer()
	: m_max_in_flight_frames(3)
{
	
}

VulkanRenderer::~VulkanRenderer()
{
	this->Shutdown();
}

void VulkanRenderer::Init(const VkInstance& vk_instance, const VulkanDeviceRequirements& logical_device_reqs, const uint32_t& max_in_flight_frames)
{
	if (m_logical_device.vk_instance != VK_NULL_HANDLE)
	{
		AURION_WARN("[Vulkan Renderer] Attempt to initialize after initialization!");
		return;
	}

	m_max_in_flight_frames = max_in_flight_frames;

	m_logical_device = VulkanDevice::Create(vk_instance, logical_device_reqs);

	// Initialize pipeline builder
	m_pipeline_builder.Initialize(&m_logical_device, m_pipelines);
}

void VulkanRenderer::Shutdown()
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

		// Cleanup all descriptor pools
		for (size_t j = 0; j < pipeline.descriptor_pools.size(); j++)
			vkDestroyDescriptorPool(m_logical_device.handle, pipeline.descriptor_pools[i], nullptr);

		// Clean up render pass
		vkDestroyRenderPass(m_logical_device.handle, pipeline.render_pass, nullptr);

		// Clean up pipeline layout
		vkDestroyPipelineLayout(m_logical_device.handle, pipeline.layout, nullptr);

		// Clean up pipeline handle
		vkDestroyPipeline(m_logical_device.handle, pipeline.handle, nullptr);
	}
	m_pipeline_builder.Cleanup();

	// Ensure all window resources have been cleaned up BEFORE the logical
	//	device is destroyed
	m_windows.clear();

	// Destroy VMA allocator
	vmaDestroyAllocator(m_logical_device.allocator);

	// Destroy logical device
	vkDestroyDevice(m_logical_device.handle, nullptr);
}

void VulkanRenderer::BeginFrame()
{
	// If rendering fails for whatever reason, remove the
	//	graphics window.
	for (auto& [id, window] : m_windows)
		if (!window.OnRender())
			m_windows_to_remove.emplace(id);
}

void VulkanRenderer::EndFrame()
{
	// If rendering fails for whatever reason, remove the
	//	graphics window.
	for (auto& [id, window] : m_windows)
		if (!window.OnUIRender())
			m_windows_to_remove.emplace(id);

	// At the end of the frame, remove any windows that
	//	have become invalidated
	for (const uint64_t& id : m_windows_to_remove)
		this->RemoveGraphicsWindow(id);
}

bool VulkanRenderer::AddWindow(const Aurion::WindowHandle& handle)
{
	if (m_windows.contains(handle.id))
		return false;

	// Emplace the new Graphics Window, and attach the
	//	window and logical device
	auto it = m_windows.emplace(handle.id, VulkanWindow());
	it.first->second.Attach(handle, &m_logical_device);
	it.first->second.SetMaxFramesInFlight(m_max_in_flight_frames);

	return true;
}

void VulkanRenderer::SetWindowEnabled(const Aurion::WindowHandle& handle, bool enabled)
{
	if (!m_windows.contains(handle.id))
		return;

	if (enabled)
		m_windows.at(handle.id).Enable();
	else
		m_windows.at(handle.id).Disable();
}

VulkanWindow* VulkanRenderer::GetGraphicsWindow(const Aurion::WindowHandle& handle)
{
	if (!m_windows.contains(handle.id))
		return nullptr;

	return &m_windows.at(handle.id);
}

VulkanWindow* VulkanRenderer::GetGraphicsWindow(const uint64_t& window_id)
{
	if (!m_windows.contains(window_id))
		return nullptr;

	return &m_windows.at(window_id);
}

bool VulkanRenderer::RemoveGraphicsWindow(const Aurion::WindowHandle& handle)
{
	if (!m_windows.contains(handle.id))
		return false;

	m_windows.erase(handle.id);
	return true;
}

bool VulkanRenderer::RemoveGraphicsWindow(const uint64_t& window_id)
{
	if (!m_windows.contains(window_id))
		return false;

	m_windows.erase(window_id);
	return true;
}

VulkanPipelineBuilder* VulkanRenderer::GetPipelineBuilder()
{
	return &m_pipeline_builder;
}

void VulkanRenderer::BindCommand(const Aurion::WindowHandle& window_handle, const std::function<void(const VulkanCommand&)>& command)
{
	// Get the graphics window
	VulkanWindow* window = this->GetGraphicsWindow(window_handle);

	// Bail if the window wasn't found
	if (!window)
	{
		AURION_ERROR("[Vulkan Renderer] Failed to submit render command: Window with id %d does not exist", window_handle.id);
		return;
	}

	// Submit the command to the window's command buffer
	window->BindRenderCommand(command);
}

void VulkanRenderer::SubmitCommand(const Aurion::WindowHandle& window_handle, const std::function<void(const VulkanCommand&)>& command)
{
	// Get the graphics window
	VulkanWindow* window = this->GetGraphicsWindow(window_handle);

	// Bail if the window wasn't found
	if (!window)
	{
		AURION_ERROR("[Vulkan Renderer] Failed to submit render command: Window with id %d does not exist", window_handle.id);
		return;
	}

	// Submit the command to the window's command buffer
	window->SubmitRenderCommand(command);
}