module;

#include <vector>

#include <glm/glm.hpp>

export module Terrain:Data;

import Resources;

export
{
	struct TerrainConfig
	{
		bool wireframe = true;
		int chunk_size = 10;
		float scale = 1.0;
	};

	struct TerrainChunk
	{
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
	};

	struct TerrainData
	{
		std::vector<TerrainChunk> chunks;
		size_t vertex_count = 0;
		size_t index_count = 0;
		bool valid = false;
	};
}