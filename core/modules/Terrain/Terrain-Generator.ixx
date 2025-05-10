module;

#include <vector>

#include <glm/glm.hpp>

export module Terrain:Generator;

import :Data;

export
{
	class TerrainGenerator
	{
	public:
		TerrainGenerator();
		~TerrainGenerator();

		TerrainChunk* RegenerateChunk(const size_t& chunk_id, const TerrainConfig& config);

		TerrainChunk* GenerateChunk(const glm::vec3& origin, const size_t& size, const TerrainConfig& config);

	private:
		std::vector<TerrainChunk> m_chunks;
	};
}