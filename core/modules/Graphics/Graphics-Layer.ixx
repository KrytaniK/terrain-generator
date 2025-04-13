export module Graphics:RenderLayer;

import :Command;

export
{
	class IRenderLayer
	{
	public:
		virtual ~IRenderLayer() = default;

		virtual void Record(const IGraphicsCommand* command) = 0;

		virtual void Enable() = 0;

		virtual void Disable() = 0;
	};
}