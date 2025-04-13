export module HelloTriangle;

import Graphics;
import Vulkan;

export
{
	class HelloTriangleLayer : public IRenderLayer
	{
	public:
		HelloTriangleLayer();
		virtual ~HelloTriangleLayer();

		virtual void Record(const IGraphicsCommand* command) override;

		virtual void Enable() override;

		virtual void Disable() override;

		void SetGraphicsPipeline(const VulkanPipeline& pipeline);

	private:
		bool m_enabled;
		const VulkanPipeline* m_pipeline;
	};
}