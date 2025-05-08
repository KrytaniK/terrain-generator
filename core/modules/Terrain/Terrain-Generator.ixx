export module Terrain:Generator;

import :Data;

export
{
	class TerrainGenerator
	{
	public:
		TerrainGenerator();
		~TerrainGenerator();

		TerrainConfig& GetConfiguration();

		TerrainData& GetTerrainData();

		void GenerateTerrain();

	private:
		TerrainData m_data;
		TerrainConfig m_config;
	};
}