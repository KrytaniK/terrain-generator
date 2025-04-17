module;

#include <cstdint>
#include <vector>
#include <span>

export module HelloSquare;

import Graphics;
import Resources;
import Vulkan;

export
{
	class HelloSquareLayer : public IRenderLayer
	{
	public:
		HelloSquareLayer();
		virtual ~HelloSquareLayer();

		void Initialize(VulkanRenderer* renderer);

		virtual void Record(const IGraphicsCommand* command) override;

		virtual void Enable() override;

		virtual void Disable() override;

	private:
		bool m_enabled;
		VulkanDevice* m_logical_device;
		VulkanPipeline m_pipeline;

		std::vector<Vertex> m_vertices;
		std::vector<uint32_t> m_indices;
		VulkanBuffer m_staging_buffer;
		VulkanBuffer m_combined_buffer;
	};
}