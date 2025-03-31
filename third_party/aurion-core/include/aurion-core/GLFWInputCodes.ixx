module;

#include <macros/AurionExport.h>

#include <cstdint>

#include <GLFW/glfw3.h>

export module Aurion.GLFW:InputCodes;

export namespace Aurion
{
	typedef enum AURION_API GLFWMouseCodes : uint64_t
	{
		// GLFW Mouse Code Mappings
		GLFW_MOUSE_LEFT = GLFW_MOUSE_BUTTON_LEFT,
		GLFW_MOUSE_RIGHT = GLFW_MOUSE_BUTTON_RIGHT,
		GLFW_MOUSE_MIDDLE = GLFW_MOUSE_BUTTON_MIDDLE,
		GLFW_MOUSE_4 = GLFW_MOUSE_BUTTON_4,
		GLFW_MOUSE_5 = GLFW_MOUSE_BUTTON_5,
		GLFW_MOUSE_6 = GLFW_MOUSE_BUTTON_6,
		GLFW_MOUSE_7 = GLFW_MOUSE_BUTTON_7,
		GLFW_MOUSE_8 = GLFW_MOUSE_BUTTON_8,

		// Extra codes not included with GLFW
		GLFW_MOUSE_SCROLL = 0x10,
		GLFW_MOUSE_POSITION = 0x20,
	} GLFWMouseCodes;
}