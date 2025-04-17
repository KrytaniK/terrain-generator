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

		void Initialize(VulkanRenderer* renderer);

		virtual void Record(const IGraphicsCommand* command) override;

		virtual void Enable() override;

		virtual void Disable() override;

	private:
		VulkanDevice* m_logical_device;
		bool m_enabled;
		VulkanPipeline m_pipeline;
	};
}