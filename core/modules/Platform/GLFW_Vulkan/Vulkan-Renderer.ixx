module;

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <queue>

#include <vulkan/vulkan.h>

export module Vulkan:Renderer;

import Graphics;

import :Device;
import :Context;
import :Pipeline;

import :Command;

export
{
	class VulkanRenderer : public IRenderer
	{
	public:
		VulkanRenderer();
		virtual ~VulkanRenderer() override;

		VulkanRenderer(const VulkanRenderer&) = delete;
		VulkanRenderer& operator=(const VulkanRenderer&) = delete;

		VulkanRenderer(VulkanRenderer&&) = default;
		VulkanRenderer& operator=(VulkanRenderer&&) = default;

		virtual void Initialize() override;

		virtual void Render() override;

		virtual VulkanContext* CreateContext(const Aurion::WindowHandle& handle) override;

		virtual VulkanContext* GetContext(const uint64_t& id) override;

		virtual bool RemoveContext(const uint64_t& id) override;

		void SetConfiguration(const VkInstance& vk_instance, const VulkanDeviceConfiguration* device_config, const uint32_t& max_in_flight_frames = 3);

		std::vector<VulkanPipeline>& GetVkPipelineBuffer();

		VulkanDevice* GetLogicalDevice();
		
	private:
		VkInstance m_vk_instance;
		const VulkanDeviceConfiguration* m_config;
		VulkanDevice m_logical_device;
		uint32_t m_max_in_flight_frames;
		std::vector<VulkanPipeline> m_pipelines;
		std::unordered_map<uint64_t, VulkanContext> m_contexts;
		std::queue<size_t> m_remove_queue;
	};
}