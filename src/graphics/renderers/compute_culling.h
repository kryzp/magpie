#pragma once

#include "assets/assets.h"

#include "../render_graph.h"
#include "../render_scene.h"

namespace gfx
{
	class MeshPass;

	class ComputeCulling {
	public:
		void init(ast::AssetManager &assets);
		void destroy();

		void add_render_stages(
			RenderGraph &graph, RenderGraphBlackboard &bb,
			const RenderScene &scene,
			const RenderSceneResources &scene_resources
		);

	private:
		const ShaderProgram *compute_frustum_culling_program;
	};
}
