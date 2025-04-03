module;

export module Graphics:Renderer;

export
{
	class IRenderer
	{
	public:
		virtual ~IRenderer() = default;

		virtual void OnInit() = 0;
		virtual void OnShutdown() = 0;

		virtual void OnFrameBegin() = 0;
		virtual void OnFrameEnd() = 0;

		virtual void OnUIRender() = 0;

		virtual void OnFramebufferResize() = 0;
	};
}