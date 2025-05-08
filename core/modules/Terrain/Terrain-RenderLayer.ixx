module;

#include <glm/gtc/matrix_transform.hpp>

export module Terrain:RenderLayer;

import Graphics;
import Vulkan;

import :Generator;
import :Data;

export
{
	struct ModelViewProjectionMatrix
	{
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 projection;
	};

	class TerrainRenderLayer : public IRenderLayer
	{
	public:
		TerrainRenderLayer();
		virtual ~TerrainRenderLayer() override;

		void Initialize(VulkanRenderer* renderer, TerrainGenerator* generator);

		virtual void Record(const IGraphicsCommand* command) override;

		virtual void Enable() override;

		virtual void Disable() override;

		void GenerateTerrainBuffers();

		void Rotate(float fov, float aspect, float near_clip, float far_clip);

	private:
		bool m_enabled;
		TerrainGenerator* m_generator;
		VulkanDevice* m_logical_device;
		VulkanPipeline m_wireframe_pipeline;
		VulkanPipeline m_normal_pipeline;

		VulkanBuffer m_staging_buffer;
		VulkanBuffer m_terrain_buffer;
		TerrainData* m_terrain_data;

		ModelViewProjectionMatrix m_mvp_matrix;
		VulkanDescriptorPool m_mvp_desc_pool;
		VulkanDescriptorSetLayout m_mvp_desc_layout;
		std::vector<VkDescriptorSet> m_mvp_desc_sets;
		std::vector<VulkanBuffer> m_mvp_buffers;
	};
}