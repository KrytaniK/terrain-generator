module;

#include <optional>
#include <cstdint>
#include <vector>

#include <functional>

#include <vulkan/vulkan.h>
#include <imgui.h>

export module Vulkan:Window;

import Graphics;
import Aurion.Window;

import :Device;
import :Swapchain;
import :Frame;

export
{
	struct VulkanWindowSurface
	{
		VkSurfaceKHR handle = VK_NULL_HANDLE;
		VulkanSwapchain swapchain{};
		VkQueue present_queue = VK_NULL_HANDLE;
		std::optional<uint32_t> present_queue_index;
	};

	class VulkanWindow : public IGraphicsWindow
	{
	public:
		VulkanWindow();
		virtual ~VulkanWindow() override;

		virtual void Attach(const Aurion::WindowHandle& handle) override;
		void Attach(const Aurion::WindowHandle& handle, VulkanDevice* logical_device);

		virtual void SetUIRenderCallback(const std::function<void()>& ui_render_fun) override;

		virtual bool OnRender() override;

		virtual bool OnUIRender() override;

		virtual void Enable() override;

		virtual void Disable() override;

		virtual bool Enabled() override;

		virtual void SetVSyncEnabled(const bool& enabled) override;

		virtual void SetMaxFramesInFlight(const uint32_t& max_in_flight_frames) override;

		void RecreateSwapchain();

	private:
		Aurion::WindowHandle m_handle; // OS Window Handle
		VulkanDevice* m_logical_device; // Vulkan Device Information
		VulkanWindowSurface m_surface; // Vulkan Surface Information
		ImGuiContext* m_imgui_context; // ImGuiContext for this window
		std::function<void()> m_ui_render_fun;// UI Render Function
		std::vector<VulkanFrame> m_frames;
		size_t m_current_frame;
		bool m_render_as_ui;
		bool m_attached;
		bool m_enabled;
		bool m_vsync_enabled;
	};
}