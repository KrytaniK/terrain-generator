module;

#include <macros/AurionExport.h>

#include <cstdint>

export module Aurion.Window:Window;

import Aurion.Input;

// TODO: Add Graphics API target to Window Config

export namespace Aurion
{
	typedef enum AURION_API WindowMode
	{
		WINDOW_MODE_WINDOWED = 0x00,
		WINDOW_MODE_FULLSCREEN_EXCLUSIVE = 0x01,
		WINDOW_MODE_FULLSCREEN_BORDERLESS = 0x02,
	} WindowMode;

	struct AURION_API WindowConfig
	{
		const char* title = nullptr;
		uint16_t width = 1280;
		uint16_t height = 760;
		WindowMode windowMode = WINDOW_MODE_WINDOWED;
		bool decorated = true;
		bool resizable = true;
		// Has 2 bytes of 'padding'
	};

	struct AURION_API WindowProperties
	{
		const char* title = nullptr;
		uint16_t width = 1280;
		uint16_t height = 760;
		uint16_t xPos = 320;
		uint16_t yPos = 320;
		WindowMode mode = WINDOW_MODE_WINDOWED;
		bool resizable = true;
		bool minimized = false;
		bool maximized = false;
		bool focused = true;
	};

	// Base interface for interacting with application 'windows'.
	class AURION_API IWindow
	{
	public:
		virtual ~IWindow() = default;

		virtual void Open(const WindowConfig& config) = 0;

		virtual void Close() = 0;

		virtual void Update(float deltaTime = 0) = 0;

		virtual void SetTitle(const char* title) = 0;

		virtual void SetMode(const WindowMode& mode) = 0;

		virtual void Resize(const uint16_t& width, const uint16_t& height) = 0;

		virtual void SetPos(const uint16_t& xPos, const uint16_t& yPos) = 0;

		virtual bool Minimize() = 0;

		virtual bool Maximize() = 0;

		virtual bool Focus() = 0;

		virtual bool ToggleDecoration() = 0;

		virtual const char* GetTitle() = 0;

		virtual uint16_t GetWidth() = 0;

		virtual uint16_t GetHeight() = 0;

		virtual void* GetNativeHandle() = 0;

		virtual const WindowProperties& GetProperties() const = 0;

		virtual void SetInputContext(IInputContext* context) = 0;

		virtual IInputContext* GetInputContext() = 0;

		virtual bool IsOpen() = 0;
		
		virtual bool IsFullscreen() = 0;
	};

	struct AURION_API WindowHandle
	{
		uint64_t id = (uint64_t)(-1);
		IWindow* window = nullptr;
	};
}