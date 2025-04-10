module;

#include <cstdint>

export module Graphics:Context;

import Aurion.Window;

export
{
	class IGraphicsContext
	{
	public:
		virtual ~IGraphicsContext() = default;

		virtual uint64_t GetContextID() = 0;

		virtual void Initialize() = 0;

		virtual void SetWindow(const Aurion::WindowHandle& handle) = 0;

		virtual void SetMaxInFlightFrames(const uint32_t& max_in_flight_frames) = 0;

		virtual void Enable() = 0;

		virtual void Disable() = 0;

		virtual bool IsEnabled() = 0;

		virtual bool SetVSyncEnabled(const bool& enabled = true) = 0;

		virtual bool RenderFrame() = 0;
	};
}