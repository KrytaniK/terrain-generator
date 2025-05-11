export module Controllers:Camera;

import Graphics;

import Resources;
import Aurion.GLFW;

export
{
	class CameraController
	{
	public:
		CameraController();
		~CameraController();

		void Update(float delta_time);

		Camera& GetCamera();

	private:
		Aurion::GLFWInputContext* m_input_context;
		Aurion::GLFWInputDevice* m_mouse;
		Aurion::GLFWInputDevice* m_keyboard;
		Aurion::GLFWAxisControl* m_mouse_position;
		Aurion::GLFWButtonControl* m_key_w;
		Aurion::GLFWButtonControl* m_key_a;
		Aurion::GLFWButtonControl* m_key_s;
		Aurion::GLFWButtonControl* m_key_d;
		Camera m_camera;
		double m_last_mouse_x;
		double m_last_mouse_y;
	};

	class CameraConfigUIOverlay : public IRenderOverlay
	{
	public:
		CameraConfigUIOverlay();
		virtual ~CameraConfigUIOverlay() override;

		void Initialize(Camera& camera);

		void Record(const IGraphicsCommand* command) override;

		void Enable() override;

		void Disable() override;

	private:
		bool m_enabled;
		Camera* m_camera;
	};
}