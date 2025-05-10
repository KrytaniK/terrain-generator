module;

#include <vector>
#include <functional>

#include <glm/glm.hpp>

export module World:Partition;

import Collision;
import Terrain;

export
{
	class WorldPartition
	{
	public:
		WorldPartition();
		WorldPartition(const WorldPartition& parent);
		WorldPartition(const glm::vec3& origin);
		WorldPartition(const glm::vec3& origin, const size_t& size);
		~WorldPartition();

		void SetOrigin(const glm::vec3& origin);

		void SetSize(const size_t& size);

		void UpdateTerrain(const TerrainChunk& chunk);

		glm::vec3 GetOrigin() const;

		size_t GetSize() const;

		TerrainChunk& GetTerrain();

		const AABB& GetBoundingBox();

		void Partition();

		void Traverse(const std::function<void(WorldPartition&)>& traverse_fun);

	private:
		bool m_enabled;
		AABB m_bounding_box;
		std::vector<WorldPartition> m_partitions;
		TerrainChunk m_terrain;
	};
}