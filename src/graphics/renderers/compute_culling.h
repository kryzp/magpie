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

	struct CullingVolume {
		enum Type {
			TYPE_FRUSTUM,
			TYPE_SPHERE
		};

		Type type;

		FrustumVolume frustum;

		Vec3 sphere_centre;
		float sphere_radius;

		CullingVolume(const FrustumVolume &volume)
			: type(TYPE_FRUSTUM)
			, frustum(volume)
		{
		}

		CullingVolume(const Vec3 &centre, float radius)
			: type(TYPE_SPHERE)
			, sphere_centre(centre)
			, sphere_radius(radius)
		{
		}
	};

	struct DrawStream {
		RenderResourceHandle indirect_buffer;
		RenderResourceHandle count_buffer;
	};
	
	class ComputeCulling {
	public:
		void init(ast::AssetManager &assets);
		void destroy();

		DrawStream cull_geometry(
			RenderGraph &graph, RenderGraphBlackboard &bb,
			const RenderScene &scene,
			const RenderSceneResources &scene_resources,
			const CullingVolume &volume
		);

	private:
		const ShaderProgram *culling_shader;
	};
}
