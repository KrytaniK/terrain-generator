module;

#include <macros/AurionExport.h>
#include <cstdint>

export module Aurion.GLFW:Driver;

import Aurion.Window;

import :Window;

export namespace Aurion
{
	class AURION_API GLFWDriver : public IWindowDriver
	{
	public:
		GLFWDriver();
		virtual ~GLFWDriver();

		virtual void Initialize(const WindowDriverConfig& config) override;

		virtual WindowHandle GetWindow(const char* title) override;
		virtual WindowHandle GetWindow(const uint64_t& id) override;

		virtual WindowHandle InitWindow(const WindowConfig& config) override;

		virtual bool RemoveWindow(const char* title) override;
		virtual bool RemoveWindow(const uint64_t& id) override;
		virtual bool RemoveWindow(IWindow* window) override;

	private:
		size_t m_max_window_count;
		GLFW_Window* m_windows;
		uint64_t* m_window_ids;
	};
}