#include <macros/AurionLog.h>

#include <functional>

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <imgui.h>

import Aurion.GLFW;
import Aurion.Input;

import Application;
import Controllers;

CameraController::CameraController()
	: m_last_mouse_x(0.0), m_last_mouse_y(0.0)
{
	m_input_context = Application::InputContext();
	m_mouse = m_input_context->GetDevice("Mouse");
	m_keyboard = m_input_context->GetDevice("Keyboard");

	if (!m_mouse)
		return;

	m_mouse_position = (Aurion::GLFWAxisControl*)m_mouse->GetControl(Aurion::GLFW_MOUSE_POSITION);
	m_key_w = (Aurion::GLFWButtonControl*)m_keyboard->GetControl(GLFW_KEY_W);
	m_key_a = (Aurion::GLFWButtonControl*)m_keyboard->GetControl(GLFW_KEY_A);
	m_key_s = (Aurion::GLFWButtonControl*)m_keyboard->GetControl(GLFW_KEY_S);
	m_key_d = (Aurion::GLFWButtonControl*)m_keyboard->GetControl(GLFW_KEY_D);

	// Read position from input
	double mouse_pos[2];
	m_mouse_position->ReadValues(mouse_pos, 2);

	float delta_x = mouse_pos[0] - m_last_mouse_x;
	float delta_y = mouse_pos[1] - m_last_mouse_y;

	m_last_mouse_x = mouse_pos[0];
	m_last_mouse_y = mouse_pos[1];
}

CameraController::~CameraController()
{
	
}

void CameraController::Update(float delta_time)
{
	// Read Keyboard input (for movement)
	bool input_left, input_right;
	bool input_down, input_up;
	m_key_a->ReadValue(&input_left);
	m_key_d->ReadValue(&input_right);
	m_key_s->ReadValue(&input_down);
	m_key_w->ReadValue(&input_up);

	// Create input vector
	glm::vec3 movement_delta(input_left - input_right, input_down - input_up, 0.f);

	// Rotate the movement delta to align with camera view
	glm::vec3 movement_dir = m_camera.rotation * movement_delta;

	// Adjust Camera position (with time scaling)
	m_camera.position += movement_dir * m_camera.movement_speed * delta_time;

	// Read mouse position from input
	double mouse_pos[2];
	m_mouse_position->ReadValues(mouse_pos, 2);

	// Calculate mouse delta
	float delta_x = (float)mouse_pos[0] - (float)m_last_mouse_x;
	float delta_y = (float)mouse_pos[1] - (float)m_last_mouse_y;

	// Apply time scaling and sensitivity
	delta_x *= -100.f * m_camera.sensitivity * delta_time;
	delta_y *= 100.f * m_camera.sensitivity * delta_time;

	// Cache position
	m_last_mouse_x = mouse_pos[0];
	m_last_mouse_y = mouse_pos[1];

	// Calculate local directional vectors
	glm::vec3 forward = m_camera.rotation * glm::vec3(0.f, -1.f, 0.f);
	glm::vec3 right = m_camera.rotation * glm::vec3(1.f, 0.f, 0.f);
	glm::vec3 up = m_camera.rotation * glm::vec3(0.f, 0.f, 1.f);

	// Calculate new rotation
	glm::quat yaw = glm::angleAxis(glm::radians(delta_x), glm::vec3(0.f, 0.f, 1.f));
	glm::quat pitch = glm::quat(1.f, 0.f, 0.f, 0.f);

	// Clamp x-axis rotation to ~180 degrees
	if (m_camera.pitch + delta_y <= 90.f && m_camera.pitch + delta_y >= -90.f)
	{
		m_camera.pitch += delta_y;
		pitch = glm::angleAxis(glm::radians(delta_y), right);
	}

	// Apply new rotation
	m_camera.rotation = glm::normalize(yaw * pitch * m_camera.rotation);

	// Update View Matrix
	m_camera.view_projection.view = glm::lookAt(
		m_camera.position,
		m_camera.position + forward,
		up
	);

	// Update Projection Matrix
	// TODO: Only Recalculate when UI configuration changes
	m_camera.view_projection.projection = glm::perspective(
		glm::radians(m_camera.fov),
		m_camera.aspect_ratio,
		m_camera.near_clip,
		m_camera.far_clip
	);

	// Projection is inverted by default
	m_camera.view_projection.projection[1][1] *= -1;
}

Camera& CameraController::GetCamera()
{
	return m_camera;
}

// -----------------------------------------------
// Camera UI Overlay
// -----------------------------------------------

CameraConfigUIOverlay::CameraConfigUIOverlay()
	: m_enabled(true)
{

}

CameraConfigUIOverlay::~CameraConfigUIOverlay()
{
}

void CameraConfigUIOverlay::Initialize(Camera& camera)
{
	m_camera = &camera;
}

void CameraConfigUIOverlay::Record(const IGraphicsCommand* command)
{
	if (!m_enabled)
		return;


	ImGui::SetNextWindowBgAlpha(0.0f);
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoScrollWithMouse |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoSavedSettings;

	ImGui::Begin("Tool Tip", nullptr, flags);
		ImGui::Text("Press 'esc' to show mouse\nMove with WASD");
	ImGui::End();

	ImGui::Begin("Camera Configuration");

		ImGui::SliderFloat("FOV", &m_camera->fov, 60.f, 120.f, "%.0f");
		ImGui::SliderFloat("Near Clip Plane", &m_camera->near_clip, 0.001f, 10.f, "%.3f");
		ImGui::SliderFloat("Far Clip Plane", &m_camera->far_clip, 10.f, 1000.f, "%.3f");
		ImGui::SliderFloat("Mouse Sensitivity", &m_camera->sensitivity, 0.01f, 1.f, "%.2f");
		ImGui::SliderFloat("Camera Move Speed", &m_camera->movement_speed, 1.f, 20.f, "%.1f");

	ImGui::End();
}

void CameraConfigUIOverlay::Enable()
{
	m_enabled = true;
}

void CameraConfigUIOverlay::Disable()
{
	m_enabled = false;
}
