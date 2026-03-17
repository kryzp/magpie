#pragma once

#include "assets/assets.h"

#include "math/vec3.h"
#include "math/vec4.h"

#include "../render_graph.h"
#include "../render_scene.h"

#include "../camera.h"

namespace gfx
{
	class MeshPass;

	struct DrawStream {
		RenderResourceHandle indirect_buffer;
		RenderResourceHandle count_buffer;
	};
	
	class ComputeCulling {
	public:
		void init(ast::AssetManager &assets);
		void destroy();

		DrawStream cull_frustum(
			RenderGraph &graph, RenderGraphBlackboard &bb,
			const RenderScene &scene,
			const RenderSceneResources &scene_resources,
			const FrustumVolume &volume
		);

		DrawStream cull_sphere(
			RenderGraph &graph, RenderGraphBlackboard &bb,
			const RenderScene &scene,
			const RenderSceneResources &scene_resources,
			const Vec3 &sphere_centre, float sphere_radius
		);

	private:
		const ShaderProgram *frustum_culling_shader;
		const ShaderProgram *sphere_culling_shader;
	};
}
