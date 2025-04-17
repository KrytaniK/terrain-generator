export module ImGuiDemo;

import Graphics;

export
{
	class ImGuiDemoOverlay : public IRenderOverlay
	{
	public:
		ImGuiDemoOverlay();
		virtual ~ImGuiDemoOverlay();

		void Record(const IGraphicsCommand* command) override;

		void Enable() override;

		void Disable() override;

	private:
		bool m_enabled;
	};
}