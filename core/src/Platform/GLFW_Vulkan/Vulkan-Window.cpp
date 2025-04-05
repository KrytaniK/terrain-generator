#include <macros/AurionLog.h>

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

	m_attached = true;
}

void VulkanWindow::Attach(const Aurion::WindowHandle& handle, VulkanDevice* logical_device)
{
	// Update internal handles
	m_logical_device = logical_device;
	m_handle = handle;

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

void VulkanWindow::OnRender()
{
	// Always update the window
	m_handle.window->Update();

	// But don't attempt to render if not enabled.
	if (!m_enabled)
		return;

	if (!m_handle.window || !m_logical_device)
	{
		AURION_ERROR(
			"[VulkanWindow] Failed to render to window. Invalid %s",
			m_handle.window == nullptr ? "OS handle." : "logical device"
		);
		return;
	}

	// Setup the current frame
	const VulkanFrame& frame = m_frames[m_current_frame];

	// Process all render commands for this window

	// If we want to render the image as a UI texture, do so in OnUIRender
	if (m_render_as_ui)
		return;

	// If not rendering as a UI texture, copy the render image to the current swapchain image
}

void VulkanWindow::OnUIRender()
{
	// Don't attempt to render if not enabled.
	if (!m_enabled)
		return;

	const VulkanFrame& frame = m_frames[m_current_frame];

	// Set ImGui context and render UI
	ImGui::SetCurrentContext(m_imgui_context);

	// If rendering as a UI texture:
	//	- transition render image into a valid format for UI rendering
	//	- Add ui render image to ui context

	if (m_ui_render_fun)
		m_ui_render_fun();

	m_current_frame = (m_current_frame + 1) % m_frames.size();
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
	return m_enabled;
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
			buffer_info.commandPool = frame.graphics_cmd_pool;
			if (vkAllocateCommandBuffers(m_logical_device->handle, &buffer_info, &frame.graphics_cmd_buffer) != VK_SUCCESS)
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
}
