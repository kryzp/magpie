#pragma once

#include "assets/assets.h"

#include "../render_graph.h"
#include "../render_scene.h"

#include "compute_culling.h"

namespace gfx
{
	struct GBuffer {
		enum AttachmentType {
			ATTACHMENT_POSITION,
			ATTACHMENT_ALBEDO,
			ATTACHMENT_NORMAL,
			ATTACHMENT_EMISSIVE,
			ATTACHMENT_METALLIC_ROUGHNESS,
			ATTACHMENT_MAX_ENUM
		};

		RenderResourceHandle attachments[ATTACHMENT_MAX_ENUM];
		RenderResourceHandle depth;
	};

	class DeferredRenderer {
	public:
		void init(Device *device, ast::AssetManager &assets);
		void destroy();

		GBuffer render_geometry(
			RenderGraph &graph, RenderGraphBlackboard &bb,
			const RenderSceneResources &scene_resources,
			const GpuBuffer *frame_data,
			const DrawStream &draw_stream
		);

		RenderResourceHandle render_lighting(
			RenderGraph &graph, RenderGraphBlackboard &bb,
			const RenderSceneResources &scene_resources,
			const GBuffer &gbuffer,
			const GpuBuffer *frame_data,
			const DrawStream &draw_stream,
			const Texture *irradiance,
			const Texture *prefilter,
			const Texture *brdf
		);

	private:
		void create_light_sphere_mesh(Device *device);

		Mesh light_sphere_mesh;

		const ShaderProgram *model_shader;
		const ShaderProgram *ambient_lighting_shader;
		const ShaderProgram *direct_lighting_point_shader;
	};
}
