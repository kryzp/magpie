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

		void render_hdr_to_skybox(RenderGraph &graph);
		
		const Mesh &get_mesh() const;
		const GpuBuffer *get_capture_transforms() const;
		const Texture *get_environment_map() const;

	private:
		Device *device;
		Mesh mesh;

		const Texture *hdr_texture;
		
		GpuBuffer *cubemap_capture_transforms;

		Texture *cubemap;

		ShaderProgram shader;
		ShaderProgram hdr_to_cubemap_shader;
	};
}
