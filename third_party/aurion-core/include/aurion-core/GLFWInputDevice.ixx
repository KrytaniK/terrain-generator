module;

#include <macros/AurionExport.h>

#include <cstdint>

export module Aurion.GLFW:InputDevice;

import Aurion.Input;
import Aurion.Memory;

import :InputControls;

export namespace Aurion
{
	typedef enum AURION_API GLFWInputDeviceClass : uint64_t
	{
		GLFW_INPUT_DEVICE_CLASS_MOUSE = 0x01,
		GLFW_INPUT_DEVICE_CLASS_KEYBOARD = 0x02,
		GLFW_INPUT_DEVICE_CLASS_GAMEPAD = 0x04,
		GLFW_INPUT_DEVICE_CLASS_JOYSTICK = 0x08,
		GLFW_INPUT_DEVICE_CLASS_NONE = 0x00,
	} GLFWInputDeviceClass;

	class AURION_API GLFWInputDevice : public IInputDevice
	{
	public:
		GLFWInputDevice();
		GLFWInputDevice(const InputDeviceInfo& info, const InputDeviceLayout& layout);
		virtual ~GLFWInputDevice() override;

		virtual const InputDeviceInfo& GetInfo() override;

		virtual const uint32_t& GetLayoutID() override;

		virtual IInputControl* GetControl(const uint64_t& input_code) override;

		virtual bool Map(const InputDeviceLayout& layout) override;

		void Initialize(const InputDeviceInfo& info, const InputDeviceLayout& layout);

	private:
		InputDeviceInfo m_info;
		InputDeviceLayout m_layout;
		uint8_t* m_state_block;
		IInputControl** m_all_controls;
		PoolAllocator m_button_allocator;
		PoolAllocator m_value_allocator;
		PoolAllocator m_axis_allocator;
	};
}