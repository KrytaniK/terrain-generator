#include <macros/AurionLog.h>

#include <set>
#include <functional>
#include <algorithm>
#include <chrono>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

// TODO:
//	Copying images to the swapchain might introduce inconsistencies between
//		image sizes. To fix this, frame images need to be re-created when the
//		swapchain gets recreated

import Vulkan;
import Aurion.Window;

VulkanContext::VulkanContext()
	: m_handle({}), m_max_frames_in_flight(0), m_current_frame(0), m_vsync_enabled(true), m_enabled(true)
{

}

VulkanContext::~VulkanContext()
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

	// Clean up VkSwapchainKHR
	vkDestroySwapchainKHR(m_logical_device->handle, m_surface.swapchain.handle, nullptr);

	// Clean up VkSurfaceKHR
	vkDestroySurfaceKHR(m_logical_device->vk_instance, m_surface.handle, nullptr);
}

uint64_t VulkanContext::GetContextID()
{
	return m_handle.id;
}

void VulkanContext::Initialize()
{
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
		AURION_CRITICAL("[Vulkan Context] Failed to generate VkSurface for window with ID: %d", m_handle.id);
		return;
	}

	if (!this->QueryPresentationSupport())
		return;

	if (!this->CreateSwapchain())
	{
		AURION_CRITICAL("[Vulkan Context] Failed to create the swapchain for window with ID: %d", m_handle.id);
		return;
	}

	if (!this->GenerateFrameData())
	{
		AURION_CRITICAL("[Vulkan Context] Failed to generate frame data for window with ID: %d", m_handle.id);
		return;
	}
}

void VulkanContext::SetWindow(const Aurion::WindowHandle& handle)
{
	m_handle = handle;
}

void VulkanContext::SetMaxInFlightFrames(const uint32_t& max_in_flight_frames)
{
	m_max_frames_in_flight = max_in_flight_frames;
}

void VulkanContext::SetLogicalDevice(VulkanDevice* device)
{
	m_logical_device = device;
}

void VulkanContext::Enable()
{
	m_enabled = true;
}

void VulkanContext::Disable()
{
	m_enabled = false;
}

bool VulkanContext::IsEnabled()
{
	return m_enabled && (m_handle.window != nullptr) && (m_logical_device != nullptr);
}

bool VulkanContext::SetVSyncEnabled(const bool& enabled)
{
	// Return if nothing changed
	if (m_vsync_enabled == enabled)
		return enabled;

	// If v-sync is being toggled, the swapchain needs to be recreated
	m_vsync_enabled = enabled;

	// Always recreate for disabled VSync, but only recreate if present mode does not match for enabled VSync
	if (!m_vsync_enabled || (m_vsync_enabled && m_surface.swapchain.present_mode != VK_PRESENT_MODE_FIFO_KHR))
		this->CreateSwapchain(m_surface.swapchain.handle);

	return m_vsync_enabled = enabled;
}

bool VulkanContext::RenderFrame()
{
	// If the window is no longer open, rendering can never occur.
	if (!m_handle.window->IsOpen())
		return false;

	// Being disabled doesn't constitute a failed frame
	if (!m_enabled)
		return true;

	const VulkanFrame& frame = m_frames[m_current_frame];

	// Reset Fences
	const VkFence fences[2] = { frame.graphics_fence, frame.compute_fence };
	vkWaitForFences(m_logical_device->handle, 2, fences, VK_TRUE, UINT64_MAX);
	vkResetFences(m_logical_device->handle, 2, fences);

	// Opt for command buffer re-use
	vkResetCommandPool(m_logical_device->handle, frame.graphics_cmd_pool, 0);
	vkResetCommandPool(m_logical_device->handle, frame.compute_cmd_pool, 0);

	// Grab the next swapchain image
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
		this->CreateSwapchain(m_surface.swapchain.handle);
		// TODO: Revalidate all frame images
	}
	else if (acquire_result != VK_SUCCESS && acquire_result == VK_SUBOPTIMAL_KHR)
	{
		AURION_CRITICAL("[Vulkan Window] Failed to acquire swapchain image!");
		return false;
	}

	// Begin recording commands
	VkCommandBufferBeginInfo begin_info{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
	};
	vkBeginCommandBuffer(frame.graphics_cmd_buffer, &begin_info);
	vkBeginCommandBuffer(frame.compute_cmd_buffer, &begin_info);

	// Transition render image to a general layout for commands
	VulkanImage::TransitionLayouts(frame.graphics_cmd_buffer, {
		VulkanImage::CreateLayoutTransition(
			frame.image.image,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_GENERAL,
			VK_PIPELINE_STAGE_2_NONE,
			VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
			0,
			VK_ACCESS_2_SHADER_WRITE_BIT | VK_ACCESS_2_SHADER_READ_BIT
		)
	});

	if (m_bound_command)
	{
		m_bound_command(VulkanCommand{
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
		});
	}

	// TODO: UI Rendering

	// Get render image and swapchain in appropriate layouts for image
	//	copy
	VulkanImage::TransitionLayouts(frame.graphics_cmd_buffer, {
		VulkanImage::CreateLayoutTransition(
			frame.image.image,
			VK_IMAGE_LAYOUT_GENERAL,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
			VK_PIPELINE_STAGE_2_TRANSFER_BIT,
			VK_ACCESS_2_SHADER_WRITE_BIT | VK_ACCESS_2_SHADER_READ_BIT,
			VK_ACCESS_2_TRANSFER_READ_BIT
		),
		VulkanImage::CreateLayoutTransition(
			m_surface.swapchain.images[m_surface.swapchain.current_image_index],
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_PIPELINE_STAGE_2_NONE,
			VK_PIPELINE_STAGE_2_TRANSFER_BIT,
			0,
			VK_ACCESS_2_SHADER_WRITE_BIT
		)
	});

	// Copy frame image to swapchain image
	VkImageCopy2 region{
		.sType = VK_STRUCTURE_TYPE_IMAGE_COPY_2,
		.srcSubresource = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.mipLevel = 0,
			.baseArrayLayer = 0,
			.layerCount = 1
		},
		.srcOffset = { 0, 0, 0 },
		.dstSubresource = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.mipLevel = 0,
			.baseArrayLayer = 0,
			.layerCount = 1
		},
		.dstOffset = { 0, 0, 0 },
		.extent = {
			.width = frame.image.extent.width,
			.height = frame.image.extent.height,
			.depth = 1
		}
	};

	VkCopyImageInfo2 copy_info{
		.sType = VK_STRUCTURE_TYPE_COPY_IMAGE_INFO_2,
		.srcImage = frame.image.image,
		.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		.dstImage = m_surface.swapchain.images[m_surface.swapchain.current_image_index],
		.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		.regionCount = 1,
		.pRegions = &region
	};

	vkCmdCopyImage2(frame.graphics_cmd_buffer, &copy_info);

	VulkanImage::TransitionLayouts(frame.graphics_cmd_buffer, {
		VulkanImage::CreateLayoutTransition(
			frame.image.image,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			VK_IMAGE_LAYOUT_GENERAL,
			VK_PIPELINE_STAGE_2_TRANSFER_BIT,
			VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
			VK_ACCESS_2_TRANSFER_READ_BIT,
			VK_ACCESS_2_SHADER_WRITE_BIT | VK_ACCESS_2_SHADER_READ_BIT
		),
		VulkanImage::CreateLayoutTransition(
			m_surface.swapchain.images[m_surface.swapchain.current_image_index],
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			VK_PIPELINE_STAGE_2_TRANSFER_BIT,
			VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
			VK_ACCESS_2_TRANSFER_WRITE_BIT,
			0
		),
	});

	// End command buffer recording
	vkEndCommandBuffer(frame.graphics_cmd_buffer);
	vkEndCommandBuffer(frame.compute_cmd_buffer);

	// Submit All Commands
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

		// Submit Compute Queue
		vkQueueSubmit2(m_logical_device->compute_queue, 1, &compute_submit_info, frame.compute_fence);

		// Submit Graphics Queue
		vkQueueSubmit2(m_logical_device->graphics_queue, 1, &graphics_submit_info, frame.graphics_fence);
	}

	// Present Image
	{
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

	m_current_frame = (m_current_frame + 1) % m_frames.size();

	return true;
}

void VulkanContext::SetPresentMode(const VkPresentModeKHR& present_mode)
{
	// Don't override vsync
	if (m_vsync_enabled && present_mode != VK_PRESENT_MODE_FIFO_KHR)
		return;

	// No need to recreate swapchain if the present mode matches
	if (m_surface.swapchain.present_mode == present_mode)
		return;

	this->CreateSwapchain(m_surface.swapchain.handle, present_mode);
}

bool VulkanContext::QueryPresentationSupport()
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
		return false;
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
		{
			AURION_ERROR("[VulkanDevice::Create] Failed to fetch graphics queue with index %d", queue_info.queueFamilyIndex);
			return false;
		}
	}

	return true;
}

bool VulkanContext::QuerySwapchainSupport()
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
		return false;
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
		return false;
	}

	return true;
}

void VulkanContext::ChooseSwapchainFormat()
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

void VulkanContext::ChooseSwapchainPresentMode(const VkPresentModeKHR& present_mode)
{
	// Default to the first available present mode
	m_surface.swapchain.present_mode = m_surface.swapchain.support.present_modes[0];

	// Prefer v-sync present mode for stability if enabled, 'unlimited' otherwise
	for (const auto& mode : m_surface.swapchain.support.present_modes)
	{
		// If a desired mode was found, set it and return
		if (mode == present_mode)
		{
			m_surface.swapchain.present_mode = mode;
			return;
		}

		// Otherwise, choose from FIFO (Vsync Enabled) or MAILBOX (Vsync Disabled)
		if ((m_vsync_enabled && mode == VK_PRESENT_MODE_FIFO_KHR) || (!m_vsync_enabled && mode == VK_PRESENT_MODE_MAILBOX_KHR))
			m_surface.swapchain.present_mode = mode;
	}
}

void VulkanContext::ChooseSwapchainExtent()
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

bool VulkanContext::CreateSwapchain(const VkSwapchainKHR old_swapchain, const VkPresentModeKHR& present_mode)
{
	if (!this->QuerySwapchainSupport())
		return false;

	this->ChooseSwapchainFormat();
	this->ChooseSwapchainPresentMode(present_mode);
	this->ChooseSwapchainExtent();

	// Generate the swapchain
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

	uint32_t queueFamilyIndices[] = { m_logical_device->graphics_queue_index.value(), m_surface.present_queue_index.value() };
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

	// Attach old swapchain (If provided, but defaults to VK_NULL_HANDLE)
	createInfo.oldSwapchain = old_swapchain;
	m_surface.swapchain.handle = VK_NULL_HANDLE;

	// Create new swapchain
	if (vkCreateSwapchainKHR(m_logical_device->handle, &createInfo, nullptr, &m_surface.swapchain.handle) != VK_SUCCESS)
	{
		AURION_ERROR("[VulkanWindow] Failed to create swap chain!");
		return false;
	}

	// Retrieve swapchain images
	uint32_t image_count;
	vkGetSwapchainImagesKHR(m_logical_device->handle, m_surface.swapchain.handle, &image_count, nullptr);
	m_surface.swapchain.images.resize(image_count);
	vkGetSwapchainImagesKHR(m_logical_device->handle, m_surface.swapchain.handle, &image_count, m_surface.swapchain.images.data());

	// Destroy old swapchain, if provided
	if (old_swapchain != VK_NULL_HANDLE)
		vkDestroySwapchainKHR(m_logical_device->handle, old_swapchain, nullptr);

	// Reset current swapchain image index
	m_surface.swapchain.current_image_index = 0;

	return true;
}

bool VulkanContext::GenerateFrameData()
{
	m_frames.resize(m_max_frames_in_flight);

	for (size_t i = 0; i < m_frames.size(); i++)
	{
		VulkanFrame& frame = m_frames[i];

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
				AURION_ERROR("[VulkanWindow] Frame %d: Failed to create graphics command pool!", i);
				return false;
			}

			// Compute Command Pool
			pool_info.queueFamilyIndex = m_logical_device->compute_queue_index.value();
			if (vkCreateCommandPool(m_logical_device->handle, &pool_info, nullptr, &frame.compute_cmd_pool) != VK_SUCCESS)
			{
				AURION_ERROR("[VulkanWindow] Frame %d: Failed to create graphics command pool!", i);
				return false;
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
				AURION_ERROR("[VulkanWindow] Frame %d: Failed to create graphics command buffer!", i);
				return false;
			}

			// Compute Command Buffer
			buffer_info.commandPool = frame.compute_cmd_pool;
			if (vkAllocateCommandBuffers(m_logical_device->handle, &buffer_info, &frame.compute_cmd_buffer) != VK_SUCCESS)
			{
				AURION_ERROR("[VulkanWindow] Frame %d: Failed to create graphics command buffer!", i);
				return false;
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
				AURION_ERROR("[VulkanWindow] Frame %d: Failed to create graphics fence!", i);
				return false;
			}

			// Compute Fence
			if (vkCreateFence(m_logical_device->handle, &fence_info, nullptr, &frame.compute_fence) != VK_SUCCESS)
			{
				AURION_ERROR("[VulkanWindow] Frame %d: Failed to create compute fence!", i);
				return false;
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
				AURION_ERROR("[VulkanWindow] Frame %d: Failed to create graphics semaphore!", i);
				return false;
			}

			// Compute Semaphore
			if (vkCreateSemaphore(m_logical_device->handle, &sem_info, nullptr, &frame.compute_semaphore) != VK_SUCCESS)
			{
				AURION_ERROR("[VulkanWindow] Frame %d: Failed to create compute semaphore!", i);
				return false;
			}

			// Swapchain Semaphore
			if (vkCreateSemaphore(m_logical_device->handle, &sem_info, nullptr, &frame.swapchain_semaphore) != VK_SUCCESS)
			{
				AURION_ERROR("[VulkanWindow] Frame %d: Failed to create swapchain semaphore!", i);
				return false;
			}
		}
	}

	return true;
}

void VulkanContext::BindRenderCommand(const std::function<void(const VulkanCommand&)>& command)
{
	m_bound_command = command;
}