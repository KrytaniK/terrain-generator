module;

#include <vector>
#include <span>
#include <memory>

#include <vulkan/vulkan.h>
#include <shaderc/shaderc.hpp>

export module Vulkan:Pipeline;

import :Device;

export
{
	typedef enum VulkanPipelineType
	{
		VULKAN_PIPELINE_GRAPHICS = 0,
		VULKAN_PIPELINE_COMPUTE = 1,
		VULKAN_PIPELINE_RAY_TRACING = 2,
		VULKAN_PIPELINE_MAX_ENUM
	} VulkanPipelineType;

	union VulkanPipelineInfo
	{
		VkComputePipelineCreateInfo compute;
		VkGraphicsPipelineCreateInfo graphics;
		VkRayTracingPipelineCreateInfoKHR raytracing;
	};

	struct VulkanPipeline
	{
		VulkanPipelineType type = VULKAN_PIPELINE_MAX_ENUM;

		// Handles
		VkPipeline handle = VK_NULL_HANDLE;
		VkPipelineLayout layout = VK_NULL_HANDLE;
		VkRenderPass render_pass = VK_NULL_HANDLE;
	};

	class IVulkanPipelineStrategy
	{
		friend class VulkanPipelineFactory;

	protected:
		virtual VulkanPipelineInfo& Build() = 0;

	public:
		virtual ~IVulkanPipelineStrategy() = default;

		virtual VulkanPipelineType GetType() = 0;

		virtual void Initialize(VulkanDevice* device) = 0;

		// Shader Configuration
		virtual IVulkanPipelineStrategy& BindShader(const VkPipelineShaderStageCreateInfo& stage_info) = 0;

		// Begins the configuration chain for a new pipeline layout.
		virtual IVulkanPipelineStrategy& ConfigurePipelineLayout(const VkPipelineLayoutCreateFlags& create_flags = 0) = 0;

		// Begins the configuration chain for a new descriptor set layout
		virtual IVulkanPipelineStrategy& AddDescriptorSetLayout(const VkDescriptorSetLayout& layout) = 0;

		// Add a push constant to the current pipeline layout
		virtual IVulkanPipelineStrategy& AddPushConstantRange(const VkPushConstantRange& range) = 0;
	};

	class VulkanPipelineFactory
	{
	public:
		VulkanPipelineFactory();
		~VulkanPipelineFactory();

		void Initialize(VulkanDevice* device, std::vector<VulkanPipeline>& pipeline_buffer);

		void Cleanup();

		template<typename T> 
		T& Configure()
		{
			static_assert(std::is_base_of_v<IVulkanPipelineStrategy, T>, "Invalid VulkanPipelineStrategy derivation!");

			m_strategies.emplace_back(std::make_unique<T>());
			m_strategies.back()->Initialize(m_logical_device);

			return *static_cast<T*>(m_strategies.back().get());
		};

		std::span<VulkanPipeline> Build();

	private:
		VulkanDevice* m_logical_device;
		std::vector<std::unique_ptr<IVulkanPipelineStrategy>> m_strategies;
		std::vector<VulkanPipeline>* m_pipeline_buffer;
	};
}