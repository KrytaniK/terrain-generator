module;

#include <vector>
#include <cstdint>

export module Graphics:Driver;

import Aurion.Window;

import :Renderer;

export
{
	class IGraphicsDriver
	{
	public:
		virtual ~IGraphicsDriver() = default;

		virtual const uint64_t GetType() = 0;

		virtual void Initialize() = 0;

		virtual IRenderer* CreateRenderer() = 0;
	};
}