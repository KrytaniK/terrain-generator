export module Graphics:RenderOverlay;

import :Command;

export
{
	class IRenderOverlay
	{
	public:
		virtual ~IRenderOverlay() = default;

		virtual void Record(const IGraphicsCommand* command) = 0;

		virtual void Enable() = 0;

		virtual void Disable() = 0;
	};
}