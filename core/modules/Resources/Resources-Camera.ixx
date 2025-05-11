module;

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

export module Resources:Camera;

export
{
	struct Camera
	{
		struct ViewProjection
		{
			glm::mat4 view = glm::mat4(1.f);
			glm::mat4 projection = glm::mat4(1.f);
		};

		glm::vec3 position = glm::vec3(-10.f, -10.f, 5.f);
		glm::quat rotation = glm::quat(1.f, 0.f, 0.f, 0.f);
		ViewProjection view_projection;
		float pitch = 0.f;
		float yaw = 0.f;
		float fov = 90.f;
		float aspect_ratio = 1920.f/1080.f;
		float near_clip = 0.01f;
		float far_clip = 1000.0f;
		float sensitivity = 1.f;
		float movement_speed = 1.f;
	};
}