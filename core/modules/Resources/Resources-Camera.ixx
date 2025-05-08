module;

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

export module Resources:Camera;

export
{
	struct Camera
	{
		glm::vec3 position;
		glm::quat rotation;
		float fov = 90;
		float aspect_ratio;
		float near_clip = 0.01f;
		float far_clip = 10.0f;
	};
}