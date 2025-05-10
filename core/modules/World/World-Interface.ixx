export module World:Interface;

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

	private:
		TerrainGenerator m_terrain_generator;
		TerrainEventListener m_terrain_listener;
		WorldPartition m_root_partition;
	};
}