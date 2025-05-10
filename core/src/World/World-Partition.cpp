#include <macros/AurionLog.h>

#include <glm/glm.hpp>

import World;

WorldPartition::WorldPartition()
{

}

WorldPartition::WorldPartition(const WorldPartition& parent)
{
	// Origin needs to be offset by some amount when partitioning
	this->SetOrigin(parent.GetOrigin());
	this->SetSize(parent.GetSize() / 2);
}

WorldPartition::WorldPartition(const glm::vec3& origin)
{
	this->SetOrigin(origin);
}

WorldPartition::WorldPartition(const glm::vec3& origin, const size_t& size)
{
	this->SetSize(size);
	this->SetOrigin(origin);
}

WorldPartition::~WorldPartition()
{

}

void WorldPartition::SetOrigin(const glm::vec3& origin)
{
	float size = m_bounding_box.x_max - m_bounding_box.x_min;

	m_bounding_box.x_min = origin.x;
	m_bounding_box.x_max = origin.x + size;
	m_bounding_box.y_min = origin.y;
	m_bounding_box.y_max = origin.y + size;
	m_bounding_box.z_min = origin.z;
	m_bounding_box.z_max = origin.z + size;
}

void WorldPartition::SetSize(const size_t& size)
{
	m_bounding_box.x_max = m_bounding_box.x_min + (float)size;
	m_bounding_box.y_max = m_bounding_box.y_min + (float)size;
	m_bounding_box.z_max = m_bounding_box.z_min + (float)size;
}

void WorldPartition::UpdateTerrain(const TerrainChunk& chunk)
{
	m_terrain = chunk;
}

glm::vec3 WorldPartition::GetOrigin() const
{
	return glm::vec3(m_bounding_box.x_min, m_bounding_box.y_min, m_bounding_box.z_min);
}

size_t WorldPartition::GetSize() const
{
	return (size_t)m_bounding_box.x_max - (size_t)m_bounding_box.x_min;
}

TerrainChunk& WorldPartition::GetTerrain()
{
	return m_terrain;
}

const AABB& WorldPartition::GetBoundingBox()
{
	return m_bounding_box;
}

void WorldPartition::Partition()
{
	if (m_partitions.size() != 0)
	{
		AURION_ERROR("Failed to partition: Sub-partitions already exist!");
		return;
	}
	size_t size = this->GetSize();
	
	// Note: This value may get floored if size is not even
	size_t new_size = size / 2;

	// Split this partition into 4 equal sized partitions (In 2 Dimensions)

	// Partition 1 (top left)
	m_partitions.emplace_back(this->GetOrigin(), new_size);

	// Partition 2 (top right)
	m_partitions.emplace_back(this->GetOrigin() + glm::vec3(new_size, 0, 0), new_size);

	// Partition 3 (bottom left)
	m_partitions.emplace_back(this->GetOrigin() + glm::vec3(0, new_size, 0), new_size);

	// Partition 4 (bottom right)
	m_partitions.emplace_back(this->GetOrigin() + glm::vec3(new_size, new_size, 0), new_size);
}

void WorldPartition::Traverse(const std::function<void(WorldPartition&)>& traverse_fun)
{
	traverse_fun(*this);

	for (size_t i = 0; i < m_partitions.size(); i++)
		m_partitions[i].Traverse(traverse_fun);
}
