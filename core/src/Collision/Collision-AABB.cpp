#include <macros/AurionLog.h>

#include <glm/glm.hpp>

import Collision;

bool IsPointInsideAABB(const glm::vec2& point, const AABB& bounding_box)
{
	return 
		point.x >= bounding_box.x_min &&
		point.x <= bounding_box.x_max &&
		point.y >= bounding_box.y_min &&
		point.y <= bounding_box.y_max
	;
}

bool IsPointInsideAABB(const glm::vec3& point, const AABB& bounding_box)
{
	return
		point.x >= bounding_box.x_min &&
		point.x <= bounding_box.x_max &&
		point.y >= bounding_box.y_min &&
		point.y <= bounding_box.y_max &&
		point.z >= bounding_box.z_min &&
		point.z <= bounding_box.z_max
	;
}

bool IntersectsAABB(const AABB& a, const AABB& b)
{
	return
		a.x_min <= b.x_max &&
		a.x_max >= b.x_min &&
		a.y_min <= b.y_max &&
		a.y_max >= b.y_min &&
		a.z_min <= b.z_max &&
		a.z_max >= b.z_min
	;
}
