module;

#include <vector>

#include <glm/glm.hpp>

export module Terrain:Data;

import Resources;

export
{
	struct TerrainConfig
	{
		int chunk_resolution = 128;
	};

	struct TerrainChunk
	{
		size_t id = 0;
		size_t size = 0;
		glm::vec3 origin;
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
	};
}