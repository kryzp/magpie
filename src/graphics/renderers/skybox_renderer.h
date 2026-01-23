#pragma once

#include "assets/assets.h"

#include "../render_graph.h"
#include "../model.h"

namespace gfx
{
	class SkyboxRenderer {
	public:
		void init(RenderGraph &graph, ast::AssetManager &assets);
		void destroy();

		void add_render_stages(
			RenderGraph &graph, RenderGraphBlackboard &bb,
			const SceneView &view,
			const GpuBuffer &frame_data
		);

	private:
		void add_create_cubemap_stage(RenderGraph &graph);

		Device *device;
		Mesh mesh;

		Texture hdr_texture;
		bool created_cubemap = false;

		GpuBuffer cubemap_capture_transforms;

		RenderResourceHandle cubemap_attachment;

		Sampler linear_sampler;

		ShaderProgram shader;
		ShaderProgram hdr_to_cubemap_shader;
	};
}
