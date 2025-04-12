export module Graphics:Command;

import Aurion.Window;

export
{
	struct IGraphicsCommand
	{
		const Aurion::WindowHandle& window_handle;
		const size_t& current_frame;
	};
}