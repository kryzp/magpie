#pragma once

#include "assets/assets.h"

#include "../render_graph.h"
#include "../render_scene.h"

namespace gfx
{
	class IBLRenderer {
	public:
		void init(ast::AssetManager &assets);
		void destroy();

		void render_brdf(
			RenderGraph &graph,
			Texture *brdf
		);
		
		void render_environment_map(
			RenderGraph &graph,
			const Texture *irradiance,
			const Texture *prefilter,
			const Texture *environment_map,
			const Mesh &skybox,
			const GpuBuffer *capture_transforms
		);

	private:
		ShaderProgram brdf_shader;
		ShaderProgram irradiance_shader;
		ShaderProgram prefilter_shader;
	};
}
