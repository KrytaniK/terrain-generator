export module Terrain:ConfigOverlay;

import Graphics;
import Vulkan;

import :Generator;
import :Events;
import :Data;

export
{
	class World;
	class TerrainConfigOverlay : public IRenderOverlay
	{
	public:
		TerrainConfigOverlay();
		virtual ~TerrainConfigOverlay() override;

		void Initialize(TerrainEventDispatcher& event_dispatcher);

		void Record(const IGraphicsCommand* command) override;

		void Enable() override;

		void Disable() override;

	private:
		bool m_enabled;
		World* world;
		TerrainEventDispatcher* m_event_dispatcher;
		TerrainEvent m_update_event;
		TerrainConfig m_config;
	};
}