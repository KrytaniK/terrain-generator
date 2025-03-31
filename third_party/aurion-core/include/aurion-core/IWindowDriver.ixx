module;

#include <macros/AurionExport.h>

#include <cstdint>

export module Aurion.Window:Driver;

import :Window;

export namespace Aurion
{
	struct AURION_API WindowDriverConfig
	{
		size_t max_window_count;
	};

	class AURION_API IWindowDriver
	{
	public:
		virtual ~IWindowDriver() = default;

		virtual void Initialize(const WindowDriverConfig& config) = 0;

		virtual WindowHandle GetWindow(const char* title) = 0;

		virtual WindowHandle GetWindow(const uint64_t& id) = 0;

		virtual WindowHandle InitWindow(const WindowConfig& config) = 0;

		virtual bool RemoveWindow(const char* title) = 0;
		virtual bool RemoveWindow(const uint64_t& id) = 0;
		virtual bool RemoveWindow(IWindow* window) = 0;
	};
}