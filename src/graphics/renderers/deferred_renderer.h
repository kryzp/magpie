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
		void init(RenderGraph &graph, ast::AssetManager &assets);
		void destroy();

		void add_render_stages(
			RenderGraph &graph, RenderGraphBlackboard &bb,
			const SceneView &scene_view,
			const GpuBuffer &frame_data,
			const EnvironmentProbe &probe, const RenderResourceHandle &brdf // TEMPORARY !!!
		);

	private:
		Device *device;

		struct GBuffer {
			enum AttachmentType {
				ATTACHMENT_POSITION,
				ATTACHMENT_ALBEDO,
				ATTACHMENT_NORMAL,
				ATTACHMENT_EMISSIVE,
				ATTACHMENT_METALLIC_ROUGHNESS,
				ATTACHMENT_LIGHTING,
				ATTACHMENT_MAX_ENUM
			};

			RenderResourceHandle attachments[ATTACHMENT_MAX_ENUM];
			RenderResourceHandle depth;
		};

		GBuffer gbuffer;

		ShaderProgram model_shader;

		ShaderProgram ambient_lighting_shader;
		ShaderProgram direct_lighting_point_shader;
		
		Sampler linear_sampler;
	};
}
