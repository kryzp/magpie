#pragma once

#include "assets/assets.h"

#include "../render_graph.h"

namespace gfx
{

class PostProcessingRenderer {
public:
	void init(Device *device, ast::AssetManager &assets, RenderGraph &graph);
	void destroy();

	void add_render_stages(RenderGraph &graph, RenderGraphBlackboard &bb, const SceneView &view);

	void set_exposure(float exp);

private:
	Device *device;
	
	RenderResourceHandle colour_attachment;
	RenderResourceHandle output_attachment;

	ShaderProgram shader;
	
	float exposure;
};

}
