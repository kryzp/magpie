#pragma once

#include "assets/assets.h"

#include "../render_graph.h"
#include "../model.h"

namespace gfx
{
	class SkyboxRenderer {
	public:
		void init(Device *device, ast::AssetManager &assets);
		void destroy();

		void add_render_stages(
			RenderGraph &graph, RenderGraphBlackboard &bb,
			const GpuBuffer *frame_data
		);

		void render_hdr_to_skybox(
			RenderGraph &graph,
			const Texture *hdr_texture,
			const GpuBuffer *cubemap_capture_transforms
		);
		
		const Mesh &get_mesh() const;
		const Texture *get_environment_map() const;

	private:
		Device *device;
		Mesh mesh;

		Texture *cubemap;

		const ShaderProgram *shader;
		const ShaderProgram *hdr_to_cubemap_shader;
	};
}
