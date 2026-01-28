#pragma once

#include "assets/assets.h"

#include "../render_graph.h"

namespace gfx
{
	class PostProcessingRenderer {
	public:
		void init(ast::AssetManager &assets);
		void destroy();

		void add_render_stages(
			RenderGraph &graph, RenderGraphBlackboard &bb,
			RenderResourceHandle output_attachment
		);

		float get_exposure() const;
		void set_exposure(float exp);
		
	private:
		ShaderProgram shader;
		float exposure;
	};
}
