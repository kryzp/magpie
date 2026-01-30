#pragma once

#include "assets/assets.h"

#include "../render_graph.h"
#include "../render_scene.h"

namespace gfx
{
	struct DeferredRendererInfo : public RenderGraphBlackboardData {
		GFX_DEFINE_BLACKBOARD_DATA(DeferredRendererInfo);
		RenderResourceHandle lighting;
		RenderResourceHandle depth;
	};

	class DeferredRenderer {
	public:
		void init(ast::AssetManager &assets);
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

		GBuffer gbuffer;

		const ShaderProgram *model_shader;
		const ShaderProgram *ambient_lighting_shader;
		const ShaderProgram *direct_lighting_point_shader;
	};
}
