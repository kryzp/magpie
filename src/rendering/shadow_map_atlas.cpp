#include "shadow_map_atlas.h"

#include "vulkan/core.h"
#include "vulkan/image.h"

using namespace mgp;

void ShadowMapAtlas::init(VulkanCore *core)
{
	m_atlas = new Image();
	m_atlas->allocate(
		core,
		ATLAS_SIZE, ATLAS_SIZE, 1,
		core->getDepthFormat(),
		VK_IMAGE_VIEW_TYPE_2D,
		VK_IMAGE_TILING_OPTIMAL,
		1,
		VK_SAMPLE_COUNT_1_BIT,
		false,
		false
	);
}

void ShadowMapAtlas::destroy()
{
	delete m_atlas;
}

bool ShadowMapAtlas::allocate(ShadowMapAtlas::AtlasRegion* region, unsigned quality)
{
	unsigned width = m_atlas->getWidth() >> quality;
	unsigned height = m_atlas->getHeight() >> quality;

	for (int i = 0; i < quality + 1; i++)
	{
		for (int j = 0; j < quality + 1; j++)
		{
			if (i == quality && j == quality)
			{
				// this means the last possible space it could fit
				// would be the bottom right corner, but we don't ever
				// want to completely fill the shadow map, so just
				// refuse to allocate here and exit

				return false;
			}

			AtlasRegion potential;
			potential.area.x = j * width;
			potential.area.y = i * height;
			potential.area.w = width;
			potential.area.h = height;

			bool intersection = false;

			for (auto &r : m_regions)
			{
				if (r.area.intersects(potential.area))
				{
					intersection = true;
					break;
				}
			}

			if (!intersection)
			{
				m_regions.push_back(potential);
				(*region) = potential;

				return true;
			}
		}
	}

	return false;
}

bool ShadowMapAtlas::adaptiveAlloc(AtlasRegion *region, unsigned idealQuality, int worstQuality)
{
	if (idealQuality >= worstQuality)
		return false;

	if (!allocate(region, idealQuality))
		return adaptiveAlloc(region, idealQuality + 1, worstQuality);

	return true;
}

ImageView *ShadowMapAtlas::getAtlasView()
{
	return m_atlas->getStandardView();
}

void ShadowMapAtlas::clear()
{
	m_regions.clear();
}
