module;

#include <functional>

export module Graphics:Window;

import Aurion.Window;

export
{
	class IGraphicsWindow
	{
	public:
		virtual ~IGraphicsWindow() = default;

		virtual void Attach(const Aurion::WindowHandle& handle) = 0;

		virtual void SetUIRenderCallback(const std::function<void()>& ui_render_fun) = 0;

		virtual void OnRender() = 0;

		virtual void OnUIRender() = 0;

		virtual void Enable() = 0;

		virtual void Disable() = 0;

		virtual bool Enabled() = 0;

		virtual void SetVSyncEnabled(const bool& enabled) = 0;

		virtual void SetMaxFramesInFlight(const uint32_t& max_in_flight_frames) = 0;
	};
}