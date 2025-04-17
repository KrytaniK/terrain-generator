module;

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

export module DebugLayers:Grid;

import Aurion.Window;
import Graphics;
import Vulkan;

export
{
	struct ModelViewProjectionMatrix
	{
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 projection;
	};

	struct Vec2
	{
		float x;
		float y;
	};

	struct DebugGridConfig
	{
		float line_width = 1.0f;
		float cell_size = 1.0f;
		float anti_aliasing = 1.0f;
	};

	class DebugGridLayer : public IRenderLayer
	{
	public:
		DebugGridLayer();
		virtual ~DebugGridLayer() override;

		void Initialize(const DebugGridConfig* config, VulkanRenderer* renderer, const Aurion::WindowHandle& window_handle);

		virtual void Record(const IGraphicsCommand* command) override;

		virtual void Enable() override;

		virtual void Disable() override;

	private:
		void UpdateViewMatrix(float fov, float aspect, float near_clip, float far_clip);

		void RevalidateImage(const VkExtent3D& new_extent);

	private:
		const DebugGridConfig* m_config;
		bool m_enabled;
		VulkanDevice* m_logical_device;
		VulkanPipeline m_grid_pipeline;

		ModelViewProjectionMatrix m_mvp_matrix;
		VulkanDescriptorPool m_mvp_desc_pool;
		VulkanDescriptorSetLayout m_mvp_desc_layout;
		std::vector<VkDescriptorSet> m_mvp_desc_sets;
		std::vector<VulkanBuffer> m_mvp_buffers;

		// Multi-sample image
		VulkanImage m_msaa_image;

		// Config Push Constant
		VkPushConstantRange m_config_pc;
	};
}