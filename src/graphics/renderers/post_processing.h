#pragma once

#include "assets/assets.h"

#include "../render_graph.h"

namespace gfx
{
	class PostProcessingRenderer {
	public:
		void init(RenderGraph &graph, ast::AssetManager &assets, const RenderResourceHandle &output);
		void destroy();

		void add_render_stages(RenderGraph &graph, RenderGraphBlackboard &bb, const SceneView &view);

		float get_exposure() const;
		void set_exposure(float exp);
		
	private:
		Device *device;
	
		RenderResourceHandle output_attachment;
		RenderResourceHandle colour_attachment;

		ShaderProgram shader;
	
		float exposure;
	};
}
