#pragma once

#include "assets/assets.h"

#include "../render_graph.h"
#include "../render_scene.h"

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
		RenderResourceHandle lighting;
		RenderResourceHandle depth;
	};

	struct DeferredRendererInfo : public RenderGraphBlackboardData {
		GFX_DECLARE_BLACKBOARD_DATA(DeferredRendererInfo);
		GBuffer gbuffer;
	};

	class DeferredRenderer {
	public:
		void init(Device *device, ast::AssetManager &assets);
		void destroy();

		void add_render_stages(
			RenderGraph &graph, RenderGraphBlackboard &bb,
			const MeshPass &forward_pass,
			const GpuBuffer *frame_data,
			RenderResourceHandle irradiance,
			RenderResourceHandle prefilter,
			RenderResourceHandle brdf // TEMPORARY !!!
		);

	private:
		void create_light_sphere_mesh(Device *device);

		GBuffer gbuffer;

		Mesh light_sphere_mesh;

		const ShaderProgram *model_shader;
		const ShaderProgram *ambient_lighting_shader;
		const ShaderProgram *direct_lighting_point_shader;
	};
}
