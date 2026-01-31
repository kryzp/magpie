#pragma once

#include "assets/assets.h"

#include "../render_graph.h"

namespace gfx
{
	class MeshPass;

	class ComputeCulling {
	public:
		void init(ast::AssetManager &assets);
		void destroy();

		void add_render_stages(
			RenderGraph &graph, RenderGraphBlackboard &bb,
			const RenderScene &scene
		);

	private:
		const ShaderProgram *compute_frustum_culling_program;
	};
}
