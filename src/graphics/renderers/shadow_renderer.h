#pragma once

#include "assets/assets.h"

#include "../render_graph.h"
#include "../render_scene.h"

#include "compute_culling.h"

namespace gfx
{
	class ShadowRenderer {
	public:
		static constexpr u32 MAX_SHADOW_CASTERS = 6;
		static constexpr u32 SHADOW_MAP_RESOLUTION = 2048;

		void init(Device *device, ast::AssetManager &assets);
		void destroy();

		void render_shadows(
			RenderGraph &graph, RenderGraphBlackboard &bb,
			const RenderScene &scene,
			const RenderSceneResources &scene_resources,
			ComputeCulling &culling
		);

		const GpuBuffer *get_shadow_caster_buffer() const
		{
			return caster_table_buffer;
		}

	private:
		Device *device;
		ast::AssetManager *assets;

		Texture *shadow_cubemaps[MAX_SHADOW_CASTERS];
		TextureView *shadow_cubemap_views[MAX_SHADOW_CASTERS];

		GpuBuffer *handle_table_buffer;
		GpuBuffer *caster_table_buffer;

		ast::AssetHandle depth_shader_asset;
	};

	struct ShadowRendererInfo : public RenderGraphBlackboardData {
		const GpuBuffer *shadow_caster_table;
		int shadow_map_count;
		RenderResourceHandle shadow_maps[ShadowRenderer::MAX_SHADOW_CASTERS];
	};
}
