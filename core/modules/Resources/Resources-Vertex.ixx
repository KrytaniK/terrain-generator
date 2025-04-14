module;

#include <glm/glm.hpp>

export module Resources:Vertex;

export
{
	struct Vertex
	{
		glm::vec3 position;
		glm::vec3 color;
	};
}