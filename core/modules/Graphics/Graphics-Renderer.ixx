module;

#include <span>

export module Graphics:Renderer;

import Aurion.Window;

import :Window;

export
{
	class IRenderer
	{
	public:
		virtual ~IRenderer() = default;

		virtual void BeginFrame() = 0;
		virtual void EndFrame() = 0;

		virtual bool AddWindow(const Aurion::WindowHandle& handle) = 0;

		virtual void SetWindowEnabled(const Aurion::WindowHandle& handle, bool enabled) = 0;

		virtual IGraphicsWindow* GetGraphicsWindow(const Aurion::WindowHandle& handle) = 0;
		virtual IGraphicsWindow* GetGraphicsWindow(const uint64_t& window_id) = 0;

		virtual bool RemoveGraphicsWindow(const Aurion::WindowHandle& handle) = 0;
		virtual bool RemoveGraphicsWindow(const uint64_t& window_id) = 0;
	};
}