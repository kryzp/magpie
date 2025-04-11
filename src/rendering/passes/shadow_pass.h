#pragma once

#include <vector>

#include "math/rect.h"

namespace mgp
{
	class CommandBuffer;
	class RenderInfo;
	class VulkanCore;
	class Image;
	class Scene;
	class App;

	class ShadowMapManager
	{
	public:
		constexpr static int ATLAS_SIZE = 4096;

		struct AtlasRegion
		{
			RectU area;
		};

		void init(VulkanCore *core);
		void destroy();

		// 0 - perfect (ATLAS_SIZE / 2)
		// 1 - okay (ATLAS_SIZE / 4)
		// ...
		// returns TRUE on successful alloc, FALSE if no space.
		bool allocate(ShadowMapManager::AtlasRegion* region, unsigned quality);

		// tries to allocate a light of a given quality, and if it cant
		// it recursively tries to allocate of a worse quality until it can
		// returns TRUE on successful alloc, FALSE if no space
		bool adaptiveAlloc(AtlasRegion *region, unsigned idealQuality, int worstQuality = 5);

		void clear();

		Image *getAtlas();
		const Image *getAtlas() const;

	private:
		Image *m_atlas;
		std::vector<AtlasRegion> m_regions;
	};

	// todo: have separate classes like PointLightShadowMap and CascadedShadowMap for different use cases?

	class ShadowPass
	{
	public:
		static void init(VulkanCore *core, App *app);
		static void destroy();

		static void renderShadows(Scene& scene);

		static ShadowMapManager &getShadowMapManager();

	private:
		static VulkanCore *m_core;
		static App *m_app;

		static ShadowMapManager m_shadowMaps;
	};
}
