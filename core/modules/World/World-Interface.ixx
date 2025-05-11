export module World:Interface;

import Resources;
import Terrain;

import :Partition;

export
{
	class World
	{
	public:
		World();
		~World();

		void Initialize(const TerrainConfig& initial_terrain_config, TerrainEventDispatcher& terrain_dispatcher);

		WorldPartition& GetRootPartition();

		void SetPrimaryCamera(Camera& camera);

		Camera& GetPrimaryCamera();

	private:
		TerrainGenerator m_terrain_generator;
		TerrainEventListener m_terrain_listener;
		WorldPartition m_root_partition;
		Camera* m_primary_camera;
	};
}