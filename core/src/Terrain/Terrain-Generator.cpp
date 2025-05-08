#include <macros/AurionLog.h>

#include <cmath>
#include <cstdlib>

#include <glm/glm.hpp>

import Terrain;

TerrainGenerator::TerrainGenerator()
{

}

TerrainGenerator::~TerrainGenerator()
{

}

TerrainConfig& TerrainGenerator::GetConfiguration()
{
	return m_config;
}

TerrainData& TerrainGenerator::GetTerrainData()
{
	return m_data;
}

void TerrainGenerator::GenerateTerrain()
{
	m_data.chunks.clear();

	// Generate the first chunk
	m_data.chunks.emplace_back();
	TerrainChunk& first_chunk = m_data.chunks.back();
	size_t chunk_size = (size_t)m_config.chunk_size;
	float origin_offset = m_config.scale * ((float)chunk_size / 2.f);

	// Initial 2D Vertex Positions and indices for drawing (For a flat plane)
	for (size_t i = 0; i <= chunk_size; i++)
	{
		for (size_t j = 0; j <= chunk_size; j++)
		{
			float x = (i * m_config.scale) - origin_offset;
			float y = (j * m_config.scale) - origin_offset;

			first_chunk.vertices.push_back(Vertex{
				glm::vec3(x, y, 0.f),
				glm::vec3(1.f, 1.f, 1.f) // Color
			});

			if (m_config.wireframe)
			{
				// Draw a line along the y-axis
				uint32_t current = (uint32_t)((i * (chunk_size + 1)) + (j));
				if (j < chunk_size)
				{
					first_chunk.indices.push_back(current);
					first_chunk.indices.push_back(current + 1);
				}

				// Draw a line along the x-axis
				if (i < chunk_size)
				{
					first_chunk.indices.push_back(current);
					first_chunk.indices.push_back((uint32_t)(((i + 1) * (chunk_size + 1)) + (j)));
				}
			}
			else
			{
				// Calculate the positions of the square to generate
				uint32_t top_left = (uint32_t)((i * (chunk_size + 1)) + (j));
				uint32_t top_right = (uint32_t)(top_left + 1);
				uint32_t bottom_left = (uint32_t)(((i + 1) * (chunk_size + 1)) + (j));
				uint32_t bottom_right = (uint32_t)(bottom_left + 1);

				// First Triangle
				first_chunk.indices.push_back(top_left);
				first_chunk.indices.push_back(bottom_left);
				first_chunk.indices.push_back(top_right);

				// Second Triangle
				first_chunk.indices.push_back(top_right);
				first_chunk.indices.push_back(bottom_left);
				first_chunk.indices.push_back(bottom_right);
			}
		}
	}

	m_data.vertex_count = first_chunk.vertices.size();
	m_data.index_count = first_chunk.indices.size();
	m_data.valid = false;
}
