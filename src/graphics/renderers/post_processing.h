#pragma once

#include "assets/assets.h"

#include "../render_graph.h"

namespace gfx
{
	class PostProcessingRenderer {
	public:
		void init(Device *device, ast::AssetManager &assets, RenderGraph &graph);
		void destroy();

		void add_render_stages(RenderGraph &graph, RenderGraphBlackboard &bb, const SceneView &view, const RenderResourceHandle &skybox);

		void set_exposure(float exp);
	
		RenderResourceHandle output_attachment;

	private:
		Device *device;
	
		RenderResourceHandle colour_attachment;

		ShaderProgram shader;
	
		float exposure;
	};
}
