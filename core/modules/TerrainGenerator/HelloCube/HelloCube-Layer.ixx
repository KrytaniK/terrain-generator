module;

#include <cstdint>
#include <vector>
#include <span>

#include <glm/glm.hpp>

export module HelloCube;

import Resources;
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

	class HelloCubeLayer : public IRenderLayer
	{
	public:
		HelloCubeLayer();
		virtual ~HelloCubeLayer();

		void Initialize(VulkanRenderer* renderer);

		virtual void Record(const IGraphicsCommand* command) override;

		virtual void Enable() override;

		virtual void Disable() override;

	private:
		void Rotate(float fov, float aspect, float near_clip, float far_clip);

	private:
		bool m_enabled;
		VulkanDevice* m_logical_device;
		VulkanPipeline m_pipeline;

		std::vector<Vertex> m_vertices;
		std::vector<uint32_t> m_indices;
		VulkanBuffer m_staging_buffer;
		VulkanBuffer m_combined_buffer;

		ModelViewProjectionMatrix m_mvp_matrix;

		VulkanDescriptorPool m_mvp_desc_pool;
		VulkanDescriptorSetLayout m_mvp_desc_layout;
		std::vector<VkDescriptorSet> m_mvp_desc_sets;
		std::vector<VulkanBuffer> m_mvp_buffers;
	};
}