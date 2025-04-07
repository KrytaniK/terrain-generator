#include <macros/AurionLog.h>

#include <set>
#include <functional>
#include <algorithm>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

import Vulkan;
import Aurion.Window;

VulkanWindow::VulkanWindow()
	: m_handle({}), m_logical_device(nullptr), m_surface({}), m_imgui_context(nullptr), 
		m_current_frame(0), m_ui_render_fun(nullptr), m_render_as_ui(false),
		m_attached(false), m_enabled(true), m_vsync_enabled(true)
{

}

VulkanWindow::~VulkanWindow()
{
	// If no logical device was provided, window were never initialized.
	if (!m_logical_device)
		return;

	// Wait for logical device
	vkDeviceWaitIdle(m_logical_device->handle);

	// Clean up Frame resources
	for (auto& frame : m_frames)
	{
		// Cleanup Command Pools
		vkDestroyCommandPool(m_logical_device->handle, frame.graphics_cmd_pool, nullptr);
		vkDestroyCommandPool(m_logical_device->handle, frame.compute_cmd_pool, nullptr);

		// Cleanup frame sync
		vkDestroySemaphore(m_logical_device->handle, frame.compute_semaphore, nullptr);
		vkDestroySemaphore(m_logical_device->handle, frame.graphics_semaphore, nullptr);
		vkDestroySemaphore(m_logical_device->handle, frame.swapchain_semaphore, nullptr);
		vkDestroyFence(m_logical_device->handle, frame.compute_fence, nullptr);
		vkDestroyFence(m_logical_device->handle, frame.graphics_fence, nullptr);

		// Cleanup frame image
		vkDestroySampler(m_logical_device->handle, frame.image.sampler, nullptr);
		vkDestroyImageView(m_logical_device->handle, frame.image.view, nullptr);
		vmaDestroyImage(m_logical_device->allocator, frame.image.image, frame.image.allocation);
	}

	// Clean up VkSwapchainKHR image views
	for (auto& view : m_surface.swapchain.image_views)
		vkDestroyImageView(m_logical_device->handle, view, nullptr);

	// Clean up VkSwapchainKHR
	vkDestroySwapchainKHR(m_logical_device->handle, m_surface.swapchain.handle, nullptr);

	// Clean up VkSurfaceKHR
	vkDestroySurfaceKHR(m_logical_device->vk_instance, m_surface.handle, nullptr);
}

void VulkanWindow::Attach(const Aurion::WindowHandle& handle)
{
	// Don't attach a new window if one has already been attached
	if (m_attached)
	{
		AURION_ERROR("[VulkanWindow] Failed to attach window.");
		return;
	}

	// Update internal handle
	m_handle = handle;

	GLFWwindow* win = (GLFWwindow*)m_handle.window->GetNativeHandle();
	glfwSetWindowUserPointer(win, this);
	glfwSetWindowCloseCallback(win, [](GLFWwindow* window) {
		VulkanWindow* _this = (VulkanWindow*)glfwGetWindowUserPointer(window);

		// Disable the Vulkan Window
		_this->Disable();

		// Let the GPU finish any work.
		vkDeviceWaitIdle(_this->m_logical_device->handle);

		// Close the window
		_this->m_handle.window->Close();
	});

	// This will ALWAYS occur outside of normal rendering
	glfwSetFramebufferSizeCallback(win, [](GLFWwindow* window, int width, int height) {
		VulkanWindow* _this = (VulkanWindow*)glfwGetWindowUserPointer(window);

		_this->RecreateSwapchain();

		return;

		_this->SubmitRenderCommandImmediate([_this](const VkCommandBuffer& cmd_buffer) {
			_this->CopyImageToSwapchain(cmd_buffer, _this->m_cached_image.image, _this->m_cached_image.extent);
		});
	});

	m_attached = true;
}

void VulkanWindow::Attach(const Aurion::WindowHandle& handle, VulkanDevice* logical_device)
{
	// Update internal handles
	m_logical_device = logical_device;

	this->Attach(handle);

	if (!m_handle.window || !m_logical_device)
	{
		AURION_ERROR(
			"[VulkanWindow] Failed to initalize window. Invalid %s",
			m_handle.window == nullptr ? "OS handle." : "logical device"
		);
		return;
	}

	// Generate vulkan surface for presentation
	if (glfwCreateWindowSurface(m_logical_device->vk_instance, (GLFWwindow*)m_handle.window->GetNativeHandle(), nullptr, &m_surface.handle) != VK_SUCCESS)
	{
		AURION_CRITICAL("[VulkanWindow] Failed to generate VkSurface for window with ID: %d", m_handle.id);
		return;
	}

	// Query for presentation support
	{
		VkBool32 supported = VK_FALSE;

		// Query queue family count
		uint32_t family_count = 0; 
		vkGetPhysicalDeviceQueueFamilyProperties(m_logical_device->physical_device, &family_count, nullptr);

		// Get all queue families
		std::vector<VkQueueFamilyProperties> queueFamProps(family_count);
		vkGetPhysicalDeviceQueueFamilyProperties(m_logical_device->physical_device, &family_count, queueFamProps.data());

		// Quert support for presentation
		for (uint32_t i = 0; i < family_count; i++)
		{
			// Check for presentation support on this queue
			vkGetPhysicalDeviceSurfaceSupportKHR(m_logical_device->physical_device, i, m_surface.handle, &supported);

			// keep checking if this queue doesn't support presentation
			if (!supported)
				continue;

			// Break if we've found a valid queue
			m_surface.present_queue_index = i;
			break;
		}

		if (supported == VK_FALSE || !m_surface.present_queue_index.has_value())
		{
			AURION_CRITICAL("[VulkanWindow] Failed to generate VkSwapchain for window with ID %d. Reason: %s.", 
				m_handle.id,
				supported == VK_FALSE ? "Surface Presentation Not supported." : "Invalid Present Queue Index."
			);
			return;
		}

		// Grab the present queue, if supported
		if (supported == VK_TRUE && m_surface.present_queue_index.has_value())
		{
			VkDeviceQueueInfo2 queue_info{};
			queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2;
			queue_info.pNext = nullptr;
			queue_info.flags = 0;
			queue_info.queueIndex = 0;

			// Retrieve Present Queue
			queue_info.queueFamilyIndex = m_surface.present_queue_index.value();
			vkGetDeviceQueue2(m_logical_device->handle, &queue_info, &m_surface.present_queue);

			if (m_surface.present_queue == VK_NULL_HANDLE)
				AURION_ERROR("[VulkanDevice::Create] Failed to fetch graphics queue with index %d", queue_info.queueFamilyIndex);
		}
	}

	// Trigger a swapchain creation
	RecreateSwapchain();

	// Setup immediate commands
	if (false)
	{
		VkCommandPoolCreateInfo pool_info{};
		pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		pool_info.pNext = nullptr;
		pool_info.flags = 0;

		// Immediate Command Pool (with graphics queue)
		pool_info.queueFamilyIndex = m_logical_device->graphics_queue_index.value();
		if (vkCreateCommandPool(m_logical_device->handle, &pool_info, nullptr, &m_immediate_cmd_pool) != VK_SUCCESS)
		{
			AURION_ERROR("[VulkanWindow] Frame %d: Failed to create immediate command pool!");
			return;
		}

		VkCommandBufferAllocateInfo buffer_info{};
		buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		buffer_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		buffer_info.commandBufferCount = 1;
		buffer_info.pNext = nullptr;

		// Immediate Command Buffer
		buffer_info.commandPool = m_immediate_cmd_pool;
		if (vkAllocateCommandBuffers(m_logical_device->handle, &buffer_info, &m_immediate_cmd_buffer) != VK_SUCCESS)
		{
			AURION_ERROR("[VulkanWindow] Frame %d: Failed to create immediate command buffer!");
			return;
		}

		VkFenceCreateInfo fence_info{};
		fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fence_info.pNext = nullptr;
		fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		// Immediate Fence
		if (vkCreateFence(m_logical_device->handle, &fence_info, nullptr, &m_immediate_fence) != VK_SUCCESS)
		{
			AURION_ERROR("[VulkanWindow] Frame %d: Failed to create immediate fence!");
			return;
		}
	}

	// Setup ImGui for this window
	m_imgui_context = ImGui::CreateContext();
	ImGui::SetCurrentContext(m_imgui_context);

	m_attached = true;
}

void VulkanWindow::SetUIRenderCallback(const std::function<void()>& ui_render_fun)
{
	// Update this window's UI Render function, if provided.
	if (ui_render_fun)
		m_ui_render_fun = ui_render_fun;
}

bool VulkanWindow::OnRender()
{
	// Don't render if not enabled
	if (!this->Enabled())
		return false;

	// Setup the current frame
	const VulkanFrame& frame = m_frames[m_current_frame];

	// Reset the current frame
	this->Reset(frame);

	// Swap buffers; Bail if this fails
	if (!this->SwapBuffers(frame))
		return false;

	// Begin command buffer recording
	this->Begin(frame);

	// Transition the render image into general layout
	{
		VkImageMemoryBarrier2 barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		barrier.pNext = nullptr;

		// Set barrier masks
		barrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
		barrier.srcAccessMask = VK_ACCESS_2_MEMORY_READ_BIT;
		barrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
		barrier.dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;

		// Transition from old to new layout
		barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;

		// Create Image Subresource Range with aspect mask
		VkImageSubresourceRange sub_image{};
		sub_image.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		sub_image.baseMipLevel = 0;
		sub_image.levelCount = VK_REMAINING_MIP_LEVELS;
		sub_image.baseArrayLayer = 0;
		sub_image.layerCount = VK_REMAINING_ARRAY_LAYERS;

		// Create aspect mask
		barrier.subresourceRange = sub_image;
		barrier.image = frame.image.image;

		// Dependency struct
		VkDependencyInfo dep_info{};
		dep_info.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		dep_info.pNext = nullptr;

		// Attach image barrier
		dep_info.imageMemoryBarrierCount = 1;
		dep_info.pImageMemoryBarriers = &barrier;

		vkCmdPipelineBarrier2(frame.graphics_cmd_buffer, &dep_info);
	}

	this->Record(frame);

	// If we want to render the image as a UI texture, do so in OnUIRender
	if (m_render_as_ui)
		return true;

	this->CopyImageToSwapchain(frame.graphics_cmd_buffer, frame.image.image, frame.image.extent);

	return true;
}

bool VulkanWindow::OnUIRender()
{
	// Don't attempt to render if not enabled, or the window is no longer open
	if (!m_enabled || !m_handle.window->IsOpen())
		return false;

	const VulkanFrame& frame = m_frames[m_current_frame];

	if (m_ui_render_fun)
	{
		// Set ImGui context and render UI
		ImGui::SetCurrentContext(m_imgui_context);

		// Transition render image into a valid format for UI rendering

		if (m_render_as_ui) 
		{
			//	- Add ui render image to ui context
		}

		m_ui_render_fun();
	}

	VulkanImage::TransitionLayout(
		frame.graphics_cmd_buffer,
		m_surface.swapchain.images[m_surface.swapchain.current_image_index],
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
	);

	this->End(frame);

	this->SubmitAndPresent(frame);

	// Increase the current frame
	m_current_frame = (m_current_frame + 1) % m_frames.size();

	return true;
}

void VulkanWindow::Reset(const VulkanFrame& frame)
{
	// Reset Fences
	vkWaitForFences(m_logical_device->handle, 1, &frame.graphics_fence, VK_TRUE, UINT64_MAX);
	vkWaitForFences(m_logical_device->handle, 1, &frame.compute_fence, VK_TRUE, UINT64_MAX);
	vkResetFences(m_logical_device->handle, 1, &frame.graphics_fence);
	vkResetFences(m_logical_device->handle, 1, &frame.compute_fence);

	// Opt for command buffer re-use
	vkResetCommandPool(m_logical_device->handle, frame.graphics_cmd_pool, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
	vkResetCommandPool(m_logical_device->handle, frame.compute_cmd_pool, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
}

void VulkanWindow::Begin(const VulkanFrame& frame)
{
	VkCommandBufferBeginInfo frame_begin_info{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};

	// Begin recording commands
	vkBeginCommandBuffer(frame.graphics_cmd_buffer, &frame_begin_info);
	vkBeginCommandBuffer(frame.compute_cmd_buffer, &frame_begin_info);
}

void VulkanWindow::Record(const VulkanFrame& frame)
{
	// Setup command data
	VulkanCommand command_data{
		.window_handle = m_handle,
		.graphics_buffer = frame.graphics_cmd_buffer,
		.compute_buffer = frame.compute_cmd_buffer,
		.render_image = frame.image.image,
		.render_view = frame.image.view,
		.render_sampler = frame.image.sampler,
		.render_extent = frame.image.extent,
		.render_format = frame.image.format,
		.swapchain_extent = m_surface.swapchain.extent,
		.current_frame = m_current_frame
	};

	// Execute all temporary commands
	for (const auto& command : m_submit_commands)
		command(command_data);

	// Clear all temporary commands
	m_submit_commands.clear();

	// Execute all bound commands
	for (const auto& command : m_bound_commands)
		command(command_data);
}

void VulkanWindow::End(const VulkanFrame& frame)
{
	vkEndCommandBuffer(frame.graphics_cmd_buffer);
	vkEndCommandBuffer(frame.compute_cmd_buffer);
}

bool VulkanWindow::SwapBuffers(const VulkanFrame& frame)
{
	// Grab swapchain image
	VkResult acquire_result = vkAcquireNextImageKHR(
		m_logical_device->handle,
		m_surface.swapchain.handle,
		UINT64_MAX,
		frame.swapchain_semaphore,
		nullptr,
		&m_surface.swapchain.current_image_index
	);

	// Recreate swapchain if needed
	if (acquire_result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		AURION_ERROR("SWAPCHAIN IS OUT OF DATE");
		this->RecreateSwapchain();
	}
	else if (acquire_result != VK_SUCCESS && acquire_result == VK_SUBOPTIMAL_KHR)
	{
		AURION_CRITICAL("[Vulkan Window] Failed to acquire swapchain image!");
		return false;
	}

	return true;
}

void VulkanWindow::SubmitAndPresent(const VulkanFrame& frame)
{
	// Submit all commands
	{
		VkCommandBufferSubmitInfo graphics_cmd_buffer_info{};
		graphics_cmd_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
		graphics_cmd_buffer_info.commandBuffer = frame.graphics_cmd_buffer;

		VkCommandBufferSubmitInfo compute_cmd_buffer_info{};
		compute_cmd_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
		compute_cmd_buffer_info.commandBuffer = frame.compute_cmd_buffer;

		VkSemaphoreSubmitInfo graphics_signal_semaphore_info{};
		graphics_signal_semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
		graphics_signal_semaphore_info.stageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;
		graphics_signal_semaphore_info.semaphore = frame.graphics_semaphore;

		VkSemaphoreSubmitInfo compute_signal_semaphore_info{};
		compute_signal_semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
		compute_signal_semaphore_info.stageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
		compute_signal_semaphore_info.semaphore = frame.compute_semaphore;

		VkSemaphoreSubmitInfo wait_semaphore_info{};
		wait_semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
		wait_semaphore_info.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
		wait_semaphore_info.semaphore = frame.swapchain_semaphore;

		VkSubmitInfo2 graphics_submit_info{};
		graphics_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
		graphics_submit_info.commandBufferInfoCount = 1;
		graphics_submit_info.pCommandBufferInfos = &graphics_cmd_buffer_info;
		graphics_submit_info.signalSemaphoreInfoCount = 1;
		graphics_submit_info.pSignalSemaphoreInfos = &graphics_signal_semaphore_info;
		graphics_submit_info.waitSemaphoreInfoCount = 1;
		graphics_submit_info.pWaitSemaphoreInfos = &wait_semaphore_info;

		VkSubmitInfo2 compute_submit_info{};
		compute_submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
		compute_submit_info.commandBufferInfoCount = 1;
		compute_submit_info.pCommandBufferInfos = &compute_cmd_buffer_info;
		compute_submit_info.signalSemaphoreInfoCount = 1;
		compute_submit_info.pSignalSemaphoreInfos = &compute_signal_semaphore_info;

		// Submit Graphics Queue
		vkQueueSubmit2(m_logical_device->graphics_queue, 1, &graphics_submit_info, frame.graphics_fence);

		// Submit Compute Queue
		vkQueueSubmit2(m_logical_device->compute_queue, 1, &compute_submit_info, frame.compute_fence);
	}
	
	// Cache the image
	if (false)
	{
		// Recreate, if needed
		if (m_cached_image.extent.width != frame.image.extent.width ||
			m_cached_image.extent.height != frame.image.extent.height ||
			m_cached_image.extent.depth != frame.image.extent.depth)
		{
			// Cleanup cached image
			vkDestroySampler(m_logical_device->handle, m_cached_image.sampler, nullptr);
			vkDestroyImageView(m_logical_device->handle, m_cached_image.view, nullptr);
			vmaDestroyImage(m_logical_device->allocator, m_cached_image.image, m_cached_image.allocation);

			VulkanImageCreateInfo create_info{};
			create_info.extent = VkExtent3D{
				.width = m_surface.swapchain.extent.width,
				.height = m_surface.swapchain.extent.height,
				.depth = 1
			};

			// Ensure each image matches the swapchain format
			create_info.format = m_surface.swapchain.format.format;

			create_info.usage_flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
			create_info.usage_flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
			create_info.usage_flags |= VK_IMAGE_USAGE_STORAGE_BIT;
			create_info.usage_flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
			create_info.usage_flags |= VK_IMAGE_USAGE_SAMPLED_BIT;

			create_info.aspect_flags = VK_IMAGE_ASPECT_COLOR_BIT;

			m_cached_image = VulkanImage::Create(m_logical_device->handle, m_logical_device->allocator, create_info);
		}

		this->SubmitRenderCommandImmediate([&](const VkCommandBuffer& cmd_buffer) {
			VulkanImage::Blit(cmd_buffer, frame.image.image, m_cached_image.image, frame.image.extent, m_cached_image.extent);
		});
	}

	// Wait on graphics and comput queues to finish before presenting
	std::vector<VkSemaphore> wait_semaphores = { frame.graphics_semaphore, frame.compute_semaphore };

	// Present the image
	VkPresentInfoKHR present_info{};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.waitSemaphoreCount = (uint32_t)wait_semaphores.size();
	present_info.pWaitSemaphores = wait_semaphores.data();
	present_info.swapchainCount = 1;
	present_info.pSwapchains = &m_surface.swapchain.handle;
	present_info.pImageIndices = &m_surface.swapchain.current_image_index;

	vkQueuePresentKHR(m_surface.present_queue, &present_info);
}

void VulkanWindow::CopyImageToSwapchain(const VkCommandBuffer& cmd_buffer, const VkImage& image, const VkExtent3D& extent)
{
	// Image Layout
	VulkanImage::TransitionLayout(
		cmd_buffer,
		image,
		VK_IMAGE_LAYOUT_GENERAL,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
	);

	// Swapchain Image Layout
	VulkanImage::TransitionLayout(
		cmd_buffer,
		m_surface.swapchain.images[m_surface.swapchain.current_image_index],
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
	);

	// Copy frame image to swapchain image
	VulkanImage::Blit(
		cmd_buffer,
		image,
		m_surface.swapchain.images[m_surface.swapchain.current_image_index],
		extent,
		VkExtent3D{
			.width = m_surface.swapchain.extent.width,
			.height = m_surface.swapchain.extent.height,
		}
	);
}

void VulkanWindow::Enable()
{
	m_enabled = true;
}

void VulkanWindow::Disable()
{
	m_enabled = false;
}

bool VulkanWindow::Enabled()
{
	return m_enabled && (m_handle.window != nullptr) && (m_logical_device != nullptr);
}

void VulkanWindow::SetVSyncEnabled(const bool& enabled)
{
	// Set V-Sync
	m_vsync_enabled = enabled;

	RecreateSwapchain();
}

void VulkanWindow::SetMaxFramesInFlight(const uint32_t& max_in_flight_frames)
{
	// If we're reducing the number of in flight frames, clean up old frame resources and resize
	if (max_in_flight_frames < m_frames.size())
	{
		size_t remove_count = m_frames.size() - max_in_flight_frames;
		// Cleanup old frames
		for (size_t i = 0; i < remove_count; i++)
		{
			VulkanFrame& frame = m_frames[m_frames.size() - 1 - i];

			// Cleanup Command Pools
			vkDestroyCommandPool(m_logical_device->handle, frame.graphics_cmd_pool, nullptr);
			vkDestroyCommandPool(m_logical_device->handle, frame.compute_cmd_pool, nullptr);

			// Cleanup sync objects
			vkDestroySemaphore(m_logical_device->handle, frame.compute_semaphore, nullptr);
			vkDestroySemaphore(m_logical_device->handle, frame.graphics_semaphore, nullptr);
			vkDestroySemaphore(m_logical_device->handle, frame.swapchain_semaphore, nullptr);
			vkDestroyFence(m_logical_device->handle, frame.compute_fence, nullptr);
			vkDestroyFence(m_logical_device->handle, frame.graphics_fence, nullptr);
		
			// Cleanup render image
			vkDestroySampler(m_logical_device->handle, frame.image.sampler, nullptr);
			vkDestroyImageView(m_logical_device->handle, frame.image.view, nullptr);
			vmaDestroyImage(m_logical_device->allocator, frame.image.image, frame.image.allocation);
		}

		m_frames.resize(max_in_flight_frames);
		return;
	}

	// Otherwise, determine how many frames we need to create
	uint32_t create_count = max_in_flight_frames - (uint32_t)m_frames.size();

	// Resize
	m_frames.resize(max_in_flight_frames);
	
	// Determine the starting index for resource creation
	size_t create_index = m_frames.size() - create_count;
	
	// Generate frame resources for each new frame
	for (; create_index < m_frames.size(); create_index++)
	{
		VulkanFrame& frame = m_frames[create_index];

		// Generate Render Image
		{
			VulkanImageCreateInfo create_info{};
			create_info.extent = VkExtent3D{
				.width = m_surface.swapchain.extent.width,
				.height = m_surface.swapchain.extent.height,
				.depth = 1
			};

			// Ensure each image matches the swapchain format
			create_info.format = m_surface.swapchain.format.format;

			create_info.usage_flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
			create_info.usage_flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
			create_info.usage_flags |= VK_IMAGE_USAGE_STORAGE_BIT;
			create_info.usage_flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
			create_info.usage_flags |= VK_IMAGE_USAGE_SAMPLED_BIT;

			create_info.aspect_flags = VK_IMAGE_ASPECT_COLOR_BIT;

			frame.image = VulkanImage::Create(m_logical_device->handle, m_logical_device->allocator, create_info);
		}

		// Graphics/Compute Command Pool Creation
		{
			VkCommandPoolCreateInfo pool_info{};
			pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			pool_info.pNext = nullptr;
			pool_info.flags = 0;

			// Graphics Command Pool
			pool_info.queueFamilyIndex = m_logical_device->graphics_queue_index.value();
			if (vkCreateCommandPool(m_logical_device->handle, &pool_info, nullptr, &frame.graphics_cmd_pool) != VK_SUCCESS)
			{
				AURION_ERROR("[VulkanWindow] Frame %d: Failed to create graphics command pool!", create_index);
				return;
			}

			// Compute Command Pool
			pool_info.queueFamilyIndex = m_logical_device->compute_queue_index.value();
			if (vkCreateCommandPool(m_logical_device->handle, &pool_info, nullptr, &frame.compute_cmd_pool) != VK_SUCCESS)
			{
				AURION_ERROR("[VulkanWindow] Frame %d: Failed to create graphics command pool!", create_index);
				return;
			}
		}

		// Graphics/Compute Command Buffer Creation
		{
			VkCommandBufferAllocateInfo buffer_info{};
			buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			buffer_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			buffer_info.commandBufferCount = 1;
			buffer_info.pNext = nullptr;

			// Graphics Command Buffer
			buffer_info.commandPool = frame.graphics_cmd_pool;
			if (vkAllocateCommandBuffers(m_logical_device->handle, &buffer_info, &frame.graphics_cmd_buffer) != VK_SUCCESS)
			{
				AURION_ERROR("[VulkanWindow] Frame %d: Failed to create graphics command buffer!", create_index);
				return;
			}

			// Compute Command Buffer
			buffer_info.commandPool = frame.compute_cmd_pool;
			if (vkAllocateCommandBuffers(m_logical_device->handle, &buffer_info, &frame.compute_cmd_buffer) != VK_SUCCESS)
			{
				AURION_ERROR("[VulkanWindow] Frame %d: Failed to create graphics command buffer!", create_index);
				return;
			}
		}

		// Graphics/Compute Fences
		{
			VkFenceCreateInfo fence_info{};
			fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			fence_info.pNext = nullptr;
			fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

			// Graphics Fence
			if (vkCreateFence(m_logical_device->handle, &fence_info, nullptr, &frame.graphics_fence) != VK_SUCCESS)
			{
				AURION_ERROR("[VulkanWindow] Frame %d: Failed to create graphics fence!", create_index);
				return;
			}

			// Compute Fence
			if (vkCreateFence(m_logical_device->handle, &fence_info, nullptr, &frame.compute_fence) != VK_SUCCESS)
			{
				AURION_ERROR("[VulkanWindow] Frame %d: Failed to create compute fence!", create_index);
				return;
			}
		}

		// Graphics/Compute Semaphores
		{
			VkSemaphoreCreateInfo sem_info{};
			sem_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
			sem_info.pNext = nullptr;
			sem_info.flags = 0;

			// Graphics Semaphore
			if (vkCreateSemaphore(m_logical_device->handle, &sem_info, nullptr, &frame.graphics_semaphore) != VK_SUCCESS)
			{
				AURION_ERROR("[VulkanWindow] Frame %d: Failed to create graphics semaphore!", create_index);
				return;
			}

			// Compute Semaphore
			if (vkCreateSemaphore(m_logical_device->handle, &sem_info, nullptr, &frame.compute_semaphore) != VK_SUCCESS)
			{
				AURION_ERROR("[VulkanWindow] Frame %d: Failed to create compute semaphore!", create_index);
				return;
			}

			// Swapchain Semaphore
			if (vkCreateSemaphore(m_logical_device->handle, &sem_info, nullptr, &frame.swapchain_semaphore) != VK_SUCCESS)
			{
				AURION_ERROR("[VulkanWindow] Frame %d: Failed to create swapchain semaphore!", create_index);
				return;
			}
		}
	}
}

void VulkanWindow::RecreateSwapchain()
{
	// Wait for GPU to finish work
	vkDeviceWaitIdle(m_logical_device->handle);

	// Cleanup old swapchain resources
	if (m_surface.swapchain.handle != VK_NULL_HANDLE)
	{
		// Image views
		for (size_t i = 0; i < m_surface.swapchain.image_views.size(); i++)
			vkDestroyImageView(m_logical_device->handle, m_surface.swapchain.image_views[i], nullptr);

		// Swapchain
		vkDestroySwapchainKHR(m_logical_device->handle, m_surface.swapchain.handle, nullptr);
	}

	// Query Swapchain support and create swapchain
	{
		// Get surface capabilities
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_logical_device->physical_device, m_surface.handle, &m_surface.swapchain.support.capabilities);

		// Get surface formats and present modes
		uint32_t format_count;
		uint32_t present_mode_count;
		vkGetPhysicalDeviceSurfaceFormatsKHR(m_logical_device->physical_device, m_surface.handle, &format_count, nullptr);
		vkGetPhysicalDeviceSurfacePresentModesKHR(m_logical_device->physical_device, m_surface.handle, &present_mode_count, nullptr);

		// The number of format/present modes being 0 is a strong indicator of swapchain/surface not being supported
		if (format_count == 0 || present_mode_count == 0)
		{
			AURION_ERROR(
				"[VulkanWindow] VkSurface %s count(s) are 0. Surface presentation is likely not supported.",
				format_count == 0 ? (present_mode_count == 0 ? "Format and PresentMode" : "Format") : "PresentMode"
			);
			return;
		}

		// Get surface formats
		m_surface.swapchain.support.formats.resize(format_count);
		vkGetPhysicalDeviceSurfaceFormatsKHR(
			m_logical_device->physical_device,
			m_surface.handle,
			&format_count,
			m_surface.swapchain.support.formats.data()
		);

		// Get present modes
		m_surface.swapchain.support.present_modes.resize(present_mode_count);
		vkGetPhysicalDeviceSurfacePresentModesKHR(
			m_logical_device->physical_device,
			m_surface.handle,
			&present_mode_count,
			m_surface.swapchain.support.present_modes.data()
		);

		// Double check that requested formats and present modes are present
		if (m_surface.swapchain.support.formats.empty() || m_surface.swapchain.support.present_modes.empty())
		{
			AURION_ERROR(
				"[VulkanWindow] VkSurface %s count(s) are 0. Surface presentation is likely not supported.",
				m_surface.swapchain.support.formats.empty() == 0 ?
				(m_surface.swapchain.support.present_modes.empty() == 0 ? "Format and PresentMode" : "Format") :
				"PresentMode"
			);
			return;
		}
	}

	// Chooseing Swapchain Format
	{
		// Always default to the first available format
		m_surface.swapchain.format = m_surface.swapchain.support.formats[0];

		// Attempt to find the "best-case" format
		for (const auto& format : m_surface.swapchain.support.formats)
		{
			if (format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
				m_surface.swapchain.format = format;
		}
	}

	// Choose Swapchain Present Mode
	{
		// Default to the first available present mode
		m_surface.swapchain.present_mode = m_surface.swapchain.support.present_modes[0];
		
		// Prefer v-sync present mode for stability if enabled, 'unlimited' otherwise
		for (const auto& mode : m_surface.swapchain.support.present_modes)
		{
			if ((m_vsync_enabled && mode == VK_PRESENT_MODE_FIFO_KHR) || mode == VK_PRESENT_MODE_MAILBOX_KHR)
				m_surface.swapchain.present_mode = mode;
		}
	}

	// Choosing Extent
	{
		if (m_surface.swapchain.support.capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
			m_surface.swapchain.extent = m_surface.swapchain.support.capabilities.currentExtent;
		else
		{
			// GLFW works with both pixels and screen coordinates. Vulkan works with pixels
			// Because of this, the pixel resolution may be larger than the resolution in screen coordinates,
			//	so we need to ensure the resolutions match.

			int width, height;
			glfwGetFramebufferSize((GLFWwindow*)m_handle.window->GetNativeHandle(), &width, &height);

			m_surface.swapchain.extent = {
				std::clamp(static_cast<uint32_t>(width),
					m_surface.swapchain.support.capabilities.minImageExtent.width, m_surface.swapchain.support.capabilities.maxImageExtent.width
				),
				std::clamp(static_cast<uint32_t>(height),
					m_surface.swapchain.support.capabilities.minImageExtent.height, m_surface.swapchain.support.capabilities.maxImageExtent.height
				)
			};
		}
	}

	// Create Swapchain
	{
		// Assign the number of images we'd like to have in the swap chain, always 1 more than the minimum to avoid waiting for driver operations,
		//	and never more than the maximum.
		uint32_t imageCount = m_surface.swapchain.support.capabilities.minImageCount + 1;

		if (m_surface.swapchain.support.capabilities.maxImageCount > 0 && imageCount > m_surface.swapchain.support.capabilities.maxImageCount)
			imageCount = m_surface.swapchain.support.capabilities.maxImageCount;

		VkSwapchainCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = m_surface.handle;
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = m_surface.swapchain.format.format;
		createInfo.imageColorSpace = m_surface.swapchain.format.colorSpace;
		createInfo.imageExtent = m_surface.swapchain.extent;
		createInfo.imageArrayLayers = 1;

		// For use with dynamic rendering
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

		uint32_t queueFamilyIndices[] = { m_logical_device->graphics_queue_index.value(), m_surface.present_queue_index.value()};
		if (m_logical_device->graphics_queue_index.value() != m_surface.present_queue_index.value())
		{
			// If the graphics and presentation queues are different, share swapchain images among queue families
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else
		{
			// Otherwise, enforce an exclusive ownership policy for swapchain images
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 0;
			createInfo.pQueueFamilyIndices = nullptr;
		}

		// Used to alter image transformation before presentation
		createInfo.preTransform = m_surface.swapchain.support.capabilities.currentTransform;

		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = m_surface.swapchain.present_mode;
		createInfo.clipped = VK_TRUE; // clips pixels covered by another window

		createInfo.oldSwapchain = VK_NULL_HANDLE;

		if (vkCreateSwapchainKHR(m_logical_device->handle, &createInfo, nullptr, &m_surface.swapchain.handle) != VK_SUCCESS)
		{
			AURION_ERROR("[VulkanWindow] Failed to create swap chain!");
			return;
		}
	}

	// Create Images and Image Views
	{
		uint32_t image_count;

		// Retrieve image count from swapchain
		vkGetSwapchainImagesKHR(m_logical_device->handle, m_surface.swapchain.handle, &image_count, nullptr);

		// resize vector to match
		m_surface.swapchain.images.resize(image_count);
		m_surface.swapchain.image_views.resize(image_count);

		// retrieve images
		vkGetSwapchainImagesKHR(m_logical_device->handle, m_surface.swapchain.handle, &image_count, m_surface.swapchain.images.data());

		// Set up image view creation. NOTE: This is for VkSurface image views ONLY.
		VkImageViewCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = m_surface.swapchain.format.format;
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		// Create image views
		for (uint32_t i = 0; i < image_count; i++)
		{
			createInfo.image = m_surface.swapchain.images[i]; // apply current image to createInfo

			// Generate image view
			if (vkCreateImageView(m_logical_device->handle, &createInfo, nullptr, &m_surface.swapchain.image_views[i]) != VK_SUCCESS)
			{
				AURION_ERROR("[VulkanWindow] Failed to create image view!");
				return ;
			}
		}
	}

	// Revalidate Frame ImageData
	for (size_t i = 0; i < m_frames.size(); i++)
	{
		VulkanFrame& frame = m_frames[i];

		// Destroy old frame resources
		vkDestroySampler(m_logical_device->handle, frame.image.sampler, nullptr);
		vkDestroyImageView(m_logical_device->handle, frame.image.view, nullptr);
		vmaDestroyImage(m_logical_device->allocator, frame.image.image, frame.image.allocation);

		VulkanImageCreateInfo img_create_info{};
		img_create_info.extent = VkExtent3D{
			.width = m_surface.swapchain.extent.width,
			.height = m_surface.swapchain.extent.height,
			.depth = 1
		};

		// Ensure each image matches the swapchain format
		img_create_info.format = m_surface.swapchain.format.format;

		img_create_info.usage_flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		img_create_info.usage_flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		img_create_info.usage_flags |= VK_IMAGE_USAGE_STORAGE_BIT;
		img_create_info.usage_flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		img_create_info.usage_flags |= VK_IMAGE_USAGE_SAMPLED_BIT;

		img_create_info.aspect_flags = VK_IMAGE_ASPECT_COLOR_BIT;

		frame.image = VulkanImage::Create(m_logical_device->handle, m_logical_device->allocator, img_create_info);
	}

	// Re-enable for rendering
	m_surface.swapchain.current_image_index = 0;
}

void VulkanWindow::BindRenderCommand(const std::function<void(const VulkanCommand&)>& command)
{
	m_bound_commands.emplace_back(command);
}

void VulkanWindow::SubmitRenderCommand(const std::function<void(const VulkanCommand&)>& command)
{
	m_submit_commands.emplace_back(command);
}

void VulkanWindow::SubmitRenderCommandImmediate(const std::function<void(const VkCommandBuffer&)>& command)
{
	// Reset Fences
	vkResetFences(m_logical_device->handle, 1, &m_immediate_fence);

	// Opt for command buffer re-use
	vkResetCommandPool(m_logical_device->handle, m_immediate_cmd_pool, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
	vkResetCommandBuffer(m_immediate_cmd_buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	VkCommandBufferBeginInfo begin_info{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};

	vkBeginCommandBuffer(m_immediate_cmd_buffer, &begin_info);
	command(m_immediate_cmd_buffer);
	vkEndCommandBuffer(m_immediate_cmd_buffer);

	VkCommandBufferSubmitInfo buffer_info{};
	buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
	buffer_info.commandBuffer = m_immediate_cmd_buffer;

	VkSubmitInfo2 submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
	submitInfo.commandBufferInfoCount = 1;
	submitInfo.pCommandBufferInfos = &buffer_info;

	vkQueueSubmit2(m_logical_device->graphics_queue, 1, &submitInfo, m_immediate_fence);

	vkWaitForFences(m_logical_device->handle, 1, &m_immediate_fence, VK_TRUE, UINT64_MAX);
}
