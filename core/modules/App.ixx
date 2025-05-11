module;

#include <vector>

#include <GLFW/glfw3.h>

export module Application;

import Aurion.Application;
import Aurion.GLFW;
import Aurion.Input;

import Vulkan;

import World;
import Terrain;
import Controllers;

export
{
	class Application : public Aurion::IApplication
	{
	public:
		static Aurion::GLFWInputContext* InputContext() { 
			static Aurion::GLFWInputContext s_input_context;
			return &s_input_context; 
		};

	private:
		static void DispatchGLFWKeyCallbacks(GLFWwindow* window, int key, int scancode, int action, int mods);
		static void DispatchGLFWCursorPosCallbacks(GLFWwindow* window, double xpos, double ypos);

		inline static std::vector<GLFWkeyfun> s_key_callbacks;
		inline static std::vector<GLFWcursorposfun> s_pos_callbacks;

	public:
		Application();
		virtual ~Application() override;

		void StartAndRun() override;

	private:
		void Load();
		void Start();
		void Run();
		void Unload();

		void SetupInput();

	private:
		Aurion::GLFWDriver m_window_driver;
		Aurion::GLFWInputContext m_input_context;
		CameraController m_camera_controller;
		VulkanDriver m_vulkan_driver;
		VulkanRenderer* m_renderer;
		World m_world;
		TerrainEventDispatcher m_terrain_event_dispatcher;
		bool m_should_close;
	};
}