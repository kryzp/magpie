#pragma once

#include "assets/assets.h"

#include "../render_graph.h"

namespace gfx
{

class SkyboxRenderer {
public:
	void init(Device *device, ast::AssetManager &assets, RenderGraph &graph);
	void destroy();

	void add_render_stages(
		RenderGraph &graph, RenderGraphBlackboard &bb,
		const SceneView &view
	);

	RenderResourceHandle output_attachment;

private:
	void add_create_cubemap_stage(RenderGraph &graph);

	Device *device;
	Mesh mesh;

	Texture hdr_texture;
	bool created_cubemap = false;

	GpuBuffer frame_data_buffer;
	GpuBuffer cubemap_capture_transforms;

	RenderResourceHandle cubemap_attachment;

	Sampler linear_sampler;

	ShaderProgram shader;
	ShaderProgram hdr_to_cubemap_shader;
};

}
