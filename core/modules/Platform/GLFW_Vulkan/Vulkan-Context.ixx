module;

#include <optional>
#include <cstdint>
#include <vector>

#include <functional>

#include <vulkan/vulkan.h>
#include <imgui.h>

export module Vulkan:Context;

import Graphics;
import Aurion.Window;

import :Device;
import :Swapchain;
import :Frame;
import :Image;

import :Command;

export
{
	struct VulkanWindowSurface
	{
		VkSurfaceKHR handle = VK_NULL_HANDLE;
		VulkanSwapchain swapchain{};
		VkQueue present_queue = VK_NULL_HANDLE;
		std::optional<uint32_t> present_queue_index;
	};

	class VulkanContext : public IGraphicsContext
	{
	public:
		VulkanContext();
		virtual ~VulkanContext() override;

		virtual uint64_t GetContextID() override;

		void Initialize() override;

		virtual void SetWindow(const Aurion::WindowHandle& handle) override;

		virtual void SetMaxInFlightFrames(const uint32_t& max_in_flight_frames) override;

		virtual void Enable() override;

		virtual void Disable() override;

		virtual bool IsEnabled() override;
		
		virtual bool SetVSyncEnabled(const bool& enabled = true) override;

		virtual bool RenderFrame() override;

		void SetPresentMode(const VkPresentModeKHR& present_mode);

		void SetLogicalDevice(VulkanDevice* device);

		// Binds a render command for repeated calls.
		void BindRenderCommand(const std::function<void(const VulkanCommand&)>& command);

	private:
		bool QueryPresentationSupport();
		bool QuerySwapchainSupport();

		void ChooseSwapchainFormat();
		void ChooseSwapchainPresentMode(const VkPresentModeKHR& present_mode);
		void ChooseSwapchainExtent();

		bool CreateSwapchain(const VkSwapchainKHR old_swapchain = VK_NULL_HANDLE, const VkPresentModeKHR& present_mode = VK_PRESENT_MODE_MAX_ENUM_KHR);

		bool GenerateFrameData();
		bool RevalidateCurrentFrame();
		bool RevalidateAllFrames();

	private:
		Aurion::WindowHandle m_handle; // OS Window Handle
		uint32_t m_max_frames_in_flight;
		
		VulkanDevice* m_logical_device; // Vulkan Device Information
		VulkanWindowSurface m_surface; // Vulkan Surface Information

		std::vector<VulkanFrame> m_frames;

		std::function<void(const VulkanCommand&)> m_bound_command;

		size_t m_current_frame;
		bool m_render_as_ui;
		bool m_enabled;
		bool m_vsync_enabled;
	};
}