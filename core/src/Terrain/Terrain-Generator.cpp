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

TerrainChunk* TerrainGenerator::RegenerateChunk(const size_t& chunk_id, const TerrainConfig& config)
{
	if (chunk_id >= m_chunks.size())
	{
		AURION_ERROR("[TerrainGenerator] Failed to regenerate chunk: Invalid Chunk ID");
		return nullptr;
	}

	TerrainChunk& chunk = m_chunks.at(chunk_id);
	chunk.vertices.clear();
	chunk.indices.clear();

	// Initial 2D Vertex Positions and indices for drawing (For a flat plane)
	for (size_t i = 0; i <= config.chunk_resolution; i++)
	{
		for (size_t j = 0; j <= config.chunk_resolution; j++)
		{
			float x = chunk.origin.x - ((float)chunk.size / 2) + ((float)chunk.size * ((float)i / (float)config.chunk_resolution));
			float y = chunk.origin.y - ((float)chunk.size / 2) + ((float)chunk.size * ((float)j / (float)config.chunk_resolution));

			chunk.vertices.push_back(Vertex{
				glm::vec3(x, y, 0.f),
				glm::vec3(0.5f, 0.5f, 0.5f) // Color
				});

			/*if (config.wireframe)
			{*/
			// Draw a line along the y-axis
				uint32_t current = (uint32_t)((i * (config.chunk_resolution + 1)) + (j));
				if (j < config.chunk_resolution)
				{
					chunk.indices.push_back(current);
					chunk.indices.push_back(current + 1);
				}

				// Draw a line along the x-axis
				if (i < config.chunk_resolution)
				{
					chunk.indices.push_back(current);
					chunk.indices.push_back((uint32_t)(((i + 1) * (config.chunk_resolution + 1)) + (j)));
				}
			//}
			//else
			//{
			//	// Calculate the positions of the square to generate
			//	uint32_t top_left = (uint32_t)((i * (chunk_size + 1)) + (j));
			//	uint32_t top_right = (uint32_t)(top_left + 1);
			//	uint32_t bottom_left = (uint32_t)(((i + 1) * (chunk_size + 1)) + (j));
			//	uint32_t bottom_right = (uint32_t)(bottom_left + 1);

			//	// First Triangle
			//	chunk.indices.push_back(top_left);
			//	chunk.indices.push_back(bottom_left);
			//	chunk.indices.push_back(top_right);

			//	// Second Triangle
			//	chunk.indices.push_back(top_right);
			//	chunk.indices.push_back(bottom_left);
			//	chunk.indices.push_back(bottom_right);
			//}
		}
	}

	return &m_chunks.at(chunk_id);
}


TerrainChunk* TerrainGenerator::GenerateChunk(const glm::vec3& origin, const size_t& size, const TerrainConfig& config)
{
	// Generate the next chunk
	m_chunks.emplace_back();

	TerrainChunk& chunk = m_chunks.back();
	chunk.id = m_chunks.size() - 1;
	chunk.size = size;
	chunk.origin = origin;

	// Initial 2D Vertex Positions and indices for drawing (For a flat plane)
	for (size_t i = 0; i <= config.chunk_resolution; i++)
	{
		for (size_t j = 0; j <= config.chunk_resolution; j++)
		{
			float x = origin.x- ((float)chunk.size / 2) + ((float)size * ((float)i / (float)config.chunk_resolution));
			float y = origin.y- ((float)chunk.size / 2) + ((float)size * ((float)j / (float)config.chunk_resolution));

			chunk.vertices.push_back(Vertex{
				glm::vec3(x, y, 0.f),
				glm::vec3(0.5f, 0.5f, 0.5f) // Color
			});

			/*if (config.wireframe)
			{*/
				// Draw a line along the y-axis
				uint32_t current = (uint32_t)((i * (config.chunk_resolution + 1)) + (j));
				if (j < config.chunk_resolution)
				{
					chunk.indices.push_back(current);
					chunk.indices.push_back(current + 1);
				}

				// Draw a line along the x-axis
				if (i < config.chunk_resolution)
				{
					chunk.indices.push_back(current);
					chunk.indices.push_back((uint32_t)(((i + 1) * (config.chunk_resolution + 1)) + (j)));
				}
			//}
			//else
			//{
			//	// Calculate the positions of the square to generate
			//	uint32_t top_left = (uint32_t)((i * (chunk_size + 1)) + (j));
			//	uint32_t top_right = (uint32_t)(top_left + 1);
			//	uint32_t bottom_left = (uint32_t)(((i + 1) * (chunk_size + 1)) + (j));
			//	uint32_t bottom_right = (uint32_t)(bottom_left + 1);

			//	// First Triangle
			//	chunk.indices.push_back(top_left);
			//	chunk.indices.push_back(bottom_left);
			//	chunk.indices.push_back(top_right);

			//	// Second Triangle
			//	chunk.indices.push_back(top_right);
			//	chunk.indices.push_back(bottom_left);
			//	chunk.indices.push_back(bottom_right);
			//}
		}
	}

	return &m_chunks.back();
}
