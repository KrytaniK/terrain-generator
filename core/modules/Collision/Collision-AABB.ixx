module;

#include <glm/glm.hpp>

export module Collision:AABB;

export
{
	// Axis-Aligned Bounding Box
	struct AABB
	{
		float x_min = 0.f;
		float x_max = 0.f;
		float y_min = 0.f;
		float y_max = 0.f;
		float z_min = 0.f;
		float z_max = 0.f;
	};

	// Basic Collision Functions
	bool IsPointInsideAABB(const glm::vec2& point, const AABB& bounding_box);
	bool IsPointInsideAABB(const glm::vec3& point, const AABB& bounding_box);

	bool IntersectsAABB(const AABB& a, const AABB& b);

}