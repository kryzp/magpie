#pragma once

#include "assets/assets.h"

#include "../render_graph.h"
#include "../render_scene.h"

namespace gfx
{
	class MeshPass;
	
	struct ComputeCullingPassData : public RenderGraphBlackboardData {
		GFX_DECLARE_BLACKBOARD_DATA(ComputeCullingPassData);
		Vector<RenderResourceHandle> indirect_buffers;
		Vector<RenderResourceHandle> count_buffers;
	};

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
