module;

#include <vector>
#include <span>

#include <vulkan/vulkan.h>

export module Vulkan:Driver;

import Graphics;

import :Renderer;
import :Device;

export
{
	struct VulkanDriverConfiguration
	{
		VkApplicationInfo app_info{};
		std::vector<const char*> validation_layers{};
		VkDebugUtilsMessengerCreateInfoEXT debug_messenger_info{};
		uint32_t max_frames_in_flight = 1;
		bool enable_validation_layers = true;
		bool enable_debug_messenger = true;
	};

	class VulkanDriver : public IGraphicsDriver
	{
	public:
		VulkanDriver();
		virtual ~VulkanDriver() override;

		const uint64_t GetType() override;
		
		virtual void Initialize() override;
		void Initialize(const VulkanDriverConfiguration& config);

		virtual IRenderer* CreateRenderer(const std::vector<Aurion::WindowHandle>& windows = {}) override;
		IRenderer* CreateRenderer(std::span<Aurion::WindowHandle> windows, const VulkanDeviceRequirements& device_reqs);

	private:
		void CreateVkInstance();
		void CreateVkDebugMessenger();

	private:
		VulkanDriverConfiguration m_config;
		VkInstance m_vk_instance;
		VkDebugUtilsMessengerEXT m_vk_debug_messenger;
		std::vector<VulkanRenderer> m_renderers;
	};
}