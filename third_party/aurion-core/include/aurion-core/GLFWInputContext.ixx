module;

#include <macros/AurionExport.h>

export module Aurion.GLFW:InputContext;

import Aurion.Input;
import Aurion.Memory;

import :InputDevice;

export namespace Aurion
{
	class AURION_API GLFWInputContext : public IInputContext
	{
	public:
		GLFWInputContext();
		virtual ~GLFWInputContext() override;

		virtual GLFWInputDevice* CreateDevice(const InputDeviceInfo& info, const uint32_t& layout_id) override final;

		virtual GLFWInputDevice* CreateDevice(const InputDeviceInfo& info, const char* layout_name) override final;
		
		virtual GLFWInputDevice* CreateDevice(const InputDeviceInfo& info, const InputDeviceLayout& layout) override final;
		
		virtual GLFWInputDevice* GetDevice(const uint64_t& id) override final;
		
		virtual GLFWInputDevice* GetDevice(const char* name) override final;
		
		virtual bool RemoveDevice(const uint64_t& id) override final;
		
		virtual bool RemoveDevice(const char* name) override final;
		
		virtual bool RemoveDevice(IInputDevice* device) override final;
		
		virtual InputDeviceLayout* GetLayout(const uint32_t& layout_id) override final;
		
		virtual InputDeviceLayout* GetLayout(const char* name) override final;
		
		virtual void AddDeviceLayout(const InputDeviceLayout& layout) override final;
		
		virtual bool RemoveDeviceLayout(const uint32_t& id) override final;
		
		virtual bool RemoveDeviceLayout(const char* name) override final;
		
		virtual bool RemoveDeviceLayout(const InputDeviceLayout& layout) override final;

	private:
		size_t m_max_device_count;
		size_t m_max_layout_count;
		PoolAllocator m_device_allocator;
		PoolAllocator m_layout_allocator;
		GLFWInputDevice* m_devices;
		InputDeviceLayout* m_layouts;
	};
}