#include <macros/AurionLog.h>

#include <functional>

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

	Aurion::GLFWAxisControl* mouse_position = (Aurion::GLFWAxisControl*)m_mouse->GetControl(Aurion::GLFW_MOUSE_POSITION);

	// Read position from input
	double mouse_pos[2];
	mouse_position->ReadValues(mouse_pos, 2);

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
	Aurion::GLFWAxisControl* mouse_position = (Aurion::GLFWAxisControl*)m_mouse->GetControl(Aurion::GLFW_MOUSE_POSITION);

	// Read position from input
	double mouse_pos[2];
	mouse_position->ReadValues(mouse_pos, 2);

	float delta_x = (float)mouse_pos[0] - (float)m_last_mouse_x;
	float delta_y = (float)mouse_pos[1] - (float)m_last_mouse_y;

	delta_x *= -100.f * m_camera.sensitivity * delta_time;
	delta_y *= 100.f * m_camera.sensitivity * delta_time;

	m_last_mouse_x = mouse_pos[0];
	m_last_mouse_y = mouse_pos[1];

	glm::vec3 forward = m_camera.rotation * glm::vec3(0.f, -1.f, 0.f);
	glm::vec3 right = m_camera.rotation * glm::vec3(1.f, 0.f, 0.f);
	glm::vec3 up = m_camera.rotation * glm::vec3(0.f, 0.f, 1.f);

	glm::quat yaw = glm::angleAxis(glm::radians(delta_x), glm::vec3(0.f, 0.f, 1.f));
	glm::quat pitch = glm::quat(1.f, 0.f, 0.f, 0.f);

	if (m_camera.pitch + delta_y <= 90.f && m_camera.pitch + delta_y >= -90.f)
	{
		m_camera.pitch += delta_y;
		pitch = glm::angleAxis(glm::radians(delta_y), right);
	}

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
		ImGui::Text("Press 'esc' to show mouse");
	ImGui::End();

	ImGui::Begin("Camera Configuration");

		ImGui::SliderFloat("FOV", &m_camera->fov, 60.f, 120.f, "%.0f");
		ImGui::SliderFloat("Near Clip Plane", &m_camera->near_clip, 0.001f, 10.f, "%.3f");
		ImGui::SliderFloat("Far Clip Plane", &m_camera->far_clip, 10.f, 1000.f, "%.3f");
		ImGui::SliderFloat("Mouse Sensitivity", &m_camera->sensitivity, 0.01f, 1.f, "%.2f");

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
