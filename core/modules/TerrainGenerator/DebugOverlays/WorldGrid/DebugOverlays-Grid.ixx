module;



export module DebugOverlays:Grid;

import Graphics;
import Vulkan;

export
{
	struct DebugGridConfig;

	class DebugGridOverlay : public IRenderOverlay
	{
	public:
		DebugGridOverlay();
		virtual ~DebugGridOverlay() override;

		void Initialize(DebugGridConfig* config);

		virtual void Record(const IGraphicsCommand* command) override;

		virtual void Enable() override;

		virtual void Disable() override;

	private:
		DebugGridConfig* m_config;
		bool m_enabled;
	};
}