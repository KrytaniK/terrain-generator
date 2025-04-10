module;

#include <span>

export module Graphics:Renderer;

import Aurion.Window;

import :Context;

export
{
	class IRenderer
	{
	public:
		virtual ~IRenderer() = default;

		virtual void Initialize() = 0;

		virtual void Render() = 0;

		virtual IGraphicsContext* CreateContext(const Aurion::WindowHandle& handle) = 0;

		virtual IGraphicsContext* GetContext(const uint64_t& handle) = 0;

		virtual bool RemoveContext(const uint64_t& handle) = 0;
	};
}