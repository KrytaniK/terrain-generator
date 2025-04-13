export module HelloTriangle:Overlay;

import Graphics;

export
{
	class HelloTriangleOverlay : public IRenderOverlay
	{
	public:
		HelloTriangleOverlay();
		virtual ~HelloTriangleOverlay();

		void Record(const IGraphicsCommand* command) override;

		void Enable() override;

		void Disable() override;

	private:
		bool m_enabled;
	};
}