module;

#include <macros/AurionExport.h>

#include <cstdint>

export module Aurion.GLFW:InputControls;

import Aurion.Input;

export namespace Aurion
{
	typedef enum AURION_API GLFWInputControlType : uint32_t
	{
		GLFW_INPUT_CONTROL_BUTTON =	0x01,
		GLFW_INPUT_CONTROL_INT =	0x02,
		GLFW_INPUT_CONTROL_FLOAT =	0x04,
		GLFW_INPUT_CONTROL_DOUBLE =	0x08,
		GLFW_INPUT_CONTROL_AXIS_2D = 0x10,
		GLFW_INPUT_CONTROL_AXIS_3D = 0x20,
		GLFW_INPUT_CONTROL_AXIS_4D = 0x40,
		GLFW_INPUT_CONTROL_NONE = 0x00
	} GLFWInputControlType;

	class AURION_API GLFWInputControl : public IInputControl
	{
	public:
		GLFWInputControl();
		GLFWInputControl(uint8_t* device_state_block, const InputControlInfo& info, const InputControlLayout& layout);
		virtual ~GLFWInputControl() override;

		virtual const InputControlInfo& GetInfo() override final;

		virtual const InputControlLayout& GetLayout() override final;

		virtual const uint64_t& GetInputCode() override final;

		virtual const uint64_t& GetControlType() override final;

		virtual void Initialize(uint8_t* device_state_block, const InputControlInfo& info, const InputControlLayout& layout) override final;

		virtual bool Update(void* value, const uint16_t& value_size) override = 0;

	protected:
		InputControlInfo m_info;
		InputControlLayout m_layout;
		uint8_t* m_state_block;
	};

	class AURION_API GLFWButtonControl : public GLFWInputControl
	{
	public:
		GLFWButtonControl();
		GLFWButtonControl(uint8_t* device_state_block, const InputControlInfo& info, const InputControlLayout& layout);
		virtual ~GLFWButtonControl() override;

		virtual bool Update(void* value, const uint16_t& value_size) override;

		virtual void ReadValue(bool* out_value);
	};

	class AURION_API GLFWValueControl : public GLFWInputControl
	{
	public:
		GLFWValueControl();
		GLFWValueControl(uint8_t* device_state_block, const InputControlInfo& info, const InputControlLayout& layout);
		virtual ~GLFWValueControl() override;

		virtual bool Update(void* value, const uint16_t& value_size) override;

		virtual void ReadValue(int* out_value);
		virtual void ReadValue(float* out_value);
		virtual void ReadValue(double* out_value);
	};

	class AURION_API GLFWAxisControl : public GLFWInputControl
	{
	public:
		GLFWAxisControl();
		GLFWAxisControl(uint8_t* device_state_block, const InputControlInfo& info, const InputControlLayout& layout);
		virtual ~GLFWAxisControl() override;

		virtual bool Update(void* value, const uint16_t& value_size) override;

		virtual void ReadValues(bool* out_values, const uint8_t& read_count, uint8_t* out_count = nullptr);
		virtual void ReadValues(int* out_values, const uint8_t& read_count, uint8_t* out_count = nullptr);
		virtual void ReadValues(float* out_values, const uint8_t& read_count, uint8_t* out_count = nullptr);
		virtual void ReadValues(double* out_values, const uint8_t& read_count, uint8_t* out_count = nullptr);
	};
}