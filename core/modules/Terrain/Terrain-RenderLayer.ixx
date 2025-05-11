module;

#include <glm/gtc/matrix_transform.hpp>

export module Terrain:RenderLayer;

import Resources;
import Graphics;
import Vulkan;

import :Generator;
import :Events;
import :Data;


export
{
	struct ModelViewProjectionMatrix
	{
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 projection;
	};

	class World;
	class TerrainRenderLayer : public IRenderLayer
	{
	public:
		TerrainRenderLayer();
		virtual ~TerrainRenderLayer() override;

		void Initialize(VulkanRenderer* renderer, World& world, TerrainEventDispatcher& event_dispatcher);

		virtual void Record(const IGraphicsCommand* command) override;

		virtual void Enable() override;

		virtual void Disable() override;

		void Rotate(float fov, float aspect, float near_clip, float far_clip);

	private:
		void GenerateTerrainBuffers();

		void GenerateCameraBuffers();

		void BuildTerrainPipelines();

	private:
		VulkanRenderer* m_renderer;
		VulkanDevice* m_logical_device;
		VulkanPipeline m_wireframe_pipeline;

		TerrainGenerator* m_generator;

		World* m_world;
		Camera* m_camera;

		TerrainEventListener m_terrain_listener;

		VulkanBuffer m_staging_buffer;
		VulkanBuffer m_terrain_buffer;
		size_t m_vertex_count;
		size_t m_index_count;

		ModelViewProjectionMatrix m_mvp_matrix;
		VulkanDescriptorPool m_mvp_desc_pool;
		VulkanDescriptorSetLayout m_mvp_desc_layout;
		std::vector<VkDescriptorSet> m_mvp_desc_sets;
		std::vector<VulkanBuffer> m_mvp_buffers;
		bool m_enabled;
		bool m_revalidate_terrain;
	};
}