export module Terrain:ConfigOverlay;

import Graphics;

import :Generator;
import :Data;

export
{
	class TerrainConfigOverlay : public IRenderOverlay
	{
	public:
		TerrainConfigOverlay();
		virtual ~TerrainConfigOverlay() override;

		void Initialize(TerrainGenerator& generator);

		void Record(const IGraphicsCommand* command) override;

		void Enable() override;

		void Disable() override;

	private:
		bool m_enabled;
		TerrainGenerator* m_generator;
		TerrainConfig* m_config;
	};
}