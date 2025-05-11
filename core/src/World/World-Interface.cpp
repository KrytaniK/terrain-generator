#include <macros/AurionLog.h>

#include <glm/glm.hpp>

import World;

World::World()
	: m_terrain_generator(), m_terrain_listener(), m_root_partition(glm::vec3(0.f, 0.f, 0.f), 256)
{
	
}

World::~World()
{

}

void World::Initialize(const TerrainConfig& initial_terrain_config, TerrainEventDispatcher& terrain_dispatcher)
{
	m_terrain_listener.Bind([&](TerrainEvent* event){
		if (!event)
			return;

		// Regenerate terrain for all partitions
		m_root_partition.Traverse([&](WorldPartition& part) {
			TerrainChunk& old_chunk = part.GetTerrain();

			TerrainChunk* new_chunk = m_terrain_generator.RegenerateChunk(old_chunk.id, *event->new_config);

			part.UpdateTerrain(*new_chunk);
		});
	});

	terrain_dispatcher.AddEventListener(&m_terrain_listener);

	// Generate terrain with the base configuration
	m_root_partition.Traverse([&](WorldPartition& part) {
		TerrainChunk& old_chunk = part.GetTerrain();

		TerrainChunk* new_chunk = m_terrain_generator.GenerateChunk(part.GetOrigin(), part.GetSize(), initial_terrain_config);

		part.UpdateTerrain(*new_chunk);
	});
}

WorldPartition& World::GetRootPartition()
{
	return m_root_partition;
}

void World::SetPrimaryCamera(Camera& camera)
{
	m_primary_camera = &camera;
}

Camera& World::GetPrimaryCamera()
{
	return *m_primary_camera;
}

