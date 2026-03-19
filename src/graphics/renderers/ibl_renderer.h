#pragma once

#include "assets/assets.h"

#include "../render_graph.h"
#include "../render_scene.h"

namespace gfx
{
	class IBLRenderer {
	public:
		void init(Device *device, ast::AssetManager &assets);
		void destroy();

		void render_brdf(
			RenderGraph &graph
		);
		
		void render_environment_map(
			RenderGraph &graph,
			const Texture *environment_map,
			const Mesh &skybox,
			const GpuBuffer *capture_transforms
		);

		const Texture *get_brdf() const
		{
			return brdf;
		}

		const Texture *get_irradiance() const
		{
			return irradiance;
		}

		const Texture *get_prefilter() const
		{
			return prefilter;
		}

	private:
		Device *device;

		const ShaderProgram *brdf_shader;
		const ShaderProgram *irradiance_shader;
		const ShaderProgram *prefilter_shader;

		Texture *brdf;
		Texture *irradiance;
		Texture *prefilter;
	};
}
