#include "deferred_renderer.h"

#include "assets/shader_serializer.h"

#include "../render_scene.h"

using namespace gfx;

GFX_IMPLEMENT_BLACKBOARD_DATA(DeferredRendererInfo);

void DeferredRenderer::init(ast::AssetManager &assets)
{
	model_shader = assets.get_asset<ast::ShaderAsset>(assets.from_file_path("model"))->shader;

	ambient_lighting_shader      = assets.get_asset<ast::ShaderAsset>(assets.from_file_path("ambient_lighting"))->shader;
	direct_lighting_point_shader = assets.get_asset<ast::ShaderAsset>(assets.from_file_path("direct_lighting_point"))->shader;
}

void DeferredRenderer::destroy()
{
}

void DeferredRenderer::add_render_stages(
	RenderGraph &graph, RenderGraphBlackboard &bb,
	const MeshPass &forward_pass,
	const GpuBuffer *frame_data,
	RenderResourceHandle irradiance,
	RenderResourceHandle prefilter,
	RenderResourceHandle brdf
)
{
	struct GeometryStageData {
		RenderResourceHandle indirect_buffer;
		RenderResourceHandle instance_buffer;
	};

	graph.push_stage<GeometryStageData>(
		"Geometry",
		RenderStage::TYPE_GRAPHICS,
		[&](RenderGraphBuilder &builder, GeometryStageData &data) -> void {

			for (int i = 0; i < GBuffer::ATTACHMENT_MAX_ENUM; i++) {
				AttachmentInfo attachment_info(VK_FORMAT_R32G32B32A32_SFLOAT);
				gbuffer.attachments[i] = builder.create_texture(attachment_info);

				RenderClear clear(0.f, 0.f, 0.f, 1.f);
				builder.write_colour(gbuffer.attachments[i], SubresourceRange::all_colour(), &clear);
			}
			
			AttachmentInfo depth_info(builder.get_depth_format());
			gbuffer.depth = builder.create_texture(depth_info);

			RenderClear depth_clear(1.f, 0);
			builder.write_depth(gbuffer.depth, SubresourceRange::all_depth(), &depth_clear);
			
			data.indirect_buffer = builder.read_buffer(forward_pass.draw_indirect_buffer, GPU_BUFFER_ACCESS_INDIRECT);
			data.instance_buffer = builder.read_buffer(forward_pass.compacted_instance_buffer, GPU_BUFFER_ACCESS_GRAPHICS_READ_WRITE);
		},
		[=](const RenderContext &ctx, const RenderStageResources &resources, const GeometryStageData &data) -> void {
			CommandBuffer &cmd = ctx.cmd;
			
			const GpuBuffer *indirect_buffer = resources.get_buffer(data.indirect_buffer);
			const GpuBuffer *instance_buffer = resources.get_buffer(data.instance_buffer);

			GraphicsPipelineDef pipeline_def(model_shader);
			pipeline_def.has_depth_attachment = true;

			for (int i = 0; i < GBuffer::ATTACHMENT_MAX_ENUM; i++)
				pipeline_def.colour_attachment_formats.push_back(VK_FORMAT_R32G32B32A32_SFLOAT);

			PipelineState pipeline_st = ctx.device.fetch_pipeline(pipeline_def);

			cmd.bind_bindless(pipeline_st.bind_point, pipeline_st.layout, ctx.device.get_bindless());
			cmd.bind_pipeline(pipeline_st.bind_point, pipeline_st.pipeline);

			MeshPass &pass = ctx.scene_view.scene->get_pass(MeshPass::TYPE_FORWARD);

			for (auto &mb : pass.multi_batches) {
				const MeshPass::IndirectBatch &b = pass.batches[mb.first];
			
				RenderMesh *mesh = ctx.scene_view.scene->get_mesh(b.mesh_id);
				mesh->original->bind_indices(cmd);
			
				struct {
					u64 frame_data_buffer;
					u64 transform_buffer;
					u64 material_buffer;
					u64 vertex_buffer;
					u64 instance_buffer;
					u32 material_id;
					u32 sampler;
				} args;
			
				args.frame_data_buffer = frame_data->get_device_address();
				args.transform_buffer = ctx.scene_view.scene->get_object_buffer()->get_device_address();
				args.material_buffer = ctx.scene_view.scene->get_material_buffer()->get_device_address();
				args.vertex_buffer = mesh->original->vertex_buffer->get_device_address();
				args.instance_buffer = instance_buffer->get_device_address();
				args.material_id = b.material_id;
				args.sampler = Sampler::linear.get_bindless().sampler;

				cmd.push_constants(
					pipeline_st.layout,
					VK_SHADER_STAGE_ALL_GRAPHICS,
					sizeof(args), &args
				);

				cmd.draw_indexed_indirect(
					indirect_buffer,
					mb.first * sizeof(gpu_types::GpuIndirect),
					mb.count,
					sizeof(gpu_types::GpuIndirect)
				);
			}
		}
	);

	// ----------------------

	struct LightingStageData {
		RenderResourceHandle irradiance;
		RenderResourceHandle prefilter;
		RenderResourceHandle brdf;
	};

	graph.push_stage<LightingStageData>(
		"Lighting",
		RenderStage::TYPE_GRAPHICS,
		[&](RenderGraphBuilder &builder, LightingStageData &data) -> void {
			AttachmentInfo lighting_info(VK_FORMAT_R32G32B32A32_SFLOAT);
			gbuffer.lighting = builder.create_texture(lighting_info);

			builder.write_colour(gbuffer.lighting);
			builder.write_depth(gbuffer.depth);

			for (int i = 0; i < GBuffer::ATTACHMENT_MAX_ENUM; i++) {
				builder.read_texture(gbuffer.attachments[i]);
			}
			
			data.irradiance = builder.read_texture(irradiance);
			data.prefilter = builder.read_texture(prefilter);

			data.brdf = builder.read_texture(brdf);
		},
		[=](const RenderContext &ctx, const RenderStageResources &resources, const LightingStageData &data) -> void {
			CommandBuffer &cmd = ctx.cmd;
		
			// --- AMBIENT
			GraphicsPipelineDef ambient_pipeline_def(ambient_lighting_shader);
			ambient_pipeline_def.has_depth_attachment = true;
			ambient_pipeline_def.depth_stencil_state.depth_test_enabled = false;
			ambient_pipeline_def.depth_stencil_state.depth_write_enabled = false;
			ambient_pipeline_def.colour_attachment_formats = { VK_FORMAT_R32G32B32A32_SFLOAT };

			PipelineState ambient_pipeline_st = ctx.device.fetch_pipeline(ambient_pipeline_def);

			cmd.bind_bindless(ambient_pipeline_st.bind_point, ambient_pipeline_st.layout, ctx.device.get_bindless());
			cmd.bind_pipeline(ambient_pipeline_st.bind_point, ambient_pipeline_st.pipeline);

			struct {
				u64 frame_data_buffer;
		
				u32 position;
				u32 albedo;
				u32 normal;
				u32 material;
				u32 emissive;
			
				u32 irradiance_map;
				u32 prefilter_map;

				u32 brdf_lut;
			
				u32 linear_sampler;
			} pc_ambient;

			pc_ambient.frame_data_buffer = frame_data->get_device_address();
			
			pc_ambient.position = resources.get_texture_view(gbuffer.attachments[GBuffer::ATTACHMENT_POSITION]).get_bindless().sampled;
			pc_ambient.albedo   = resources.get_texture_view(gbuffer.attachments[GBuffer::ATTACHMENT_ALBEDO]).get_bindless().sampled;
			pc_ambient.normal   = resources.get_texture_view(gbuffer.attachments[GBuffer::ATTACHMENT_NORMAL]).get_bindless().sampled;
			pc_ambient.material = resources.get_texture_view(gbuffer.attachments[GBuffer::ATTACHMENT_EMISSIVE]).get_bindless().sampled;
			pc_ambient.emissive = resources.get_texture_view(gbuffer.attachments[GBuffer::ATTACHMENT_METALLIC_ROUGHNESS]).get_bindless().sampled;
			
			pc_ambient.irradiance_map = resources.get_texture_view(data.irradiance).get_bindless().sampled;
			pc_ambient.prefilter_map = resources.get_texture_view(data.prefilter).get_bindless().sampled;

			pc_ambient.brdf_lut = resources.get_texture_view(data.brdf).get_bindless().sampled;

			pc_ambient.linear_sampler = Sampler::linear.get_bindless().sampler;

			cmd.push_constants(ambient_pipeline_st.layout, VK_SHADER_STAGE_ALL_GRAPHICS, sizeof(pc_ambient), &pc_ambient);

			cmd.draw_vertices_n(3);

			// --- DIRECT
			/*
			GraphicsPipelineDef direct_pipeline_def(direct_lighting_point_shader);
			direct_pipeline_def.has_depth_attachment = true;
			direct_pipeline_def.depth_stencil_state.depth_test_enabled = false;
			direct_pipeline_def.depth_stencil_state.depth_write_enabled = false;
			direct_pipeline_def.cull_mode = VK_CULL_MODE_FRONT_BIT;
			direct_pipeline_def.blend_state.enabled = true;
			direct_pipeline_def.blend_state.colour.op = VK_BLEND_OP_ADD;
			direct_pipeline_def.blend_state.colour.dst = VK_BLEND_FACTOR_ONE;
			direct_pipeline_def.blend_state.colour.src = VK_BLEND_FACTOR_ONE;
			direct_pipeline_def.colour_attachment_formats = { VK_FORMAT_R32G32B32A32_SFLOAT };
		
			PipelineState direct_pipeline_st = ctx.device.fetch_pipeline(direct_pipeline_def);

			cmd.bind_bindless(direct_pipeline_st.bind_point, direct_pipeline_st.layout, ctx.device.get_bindless());
			cmd.bind_pipeline(direct_pipeline_st.bind_point, direct_pipeline_st.pipeline);
			*/
		}
	);

	// ----------------------
	
	DeferredRendererInfo info = {};
	info.lighting = gbuffer.lighting;
	info.depth = gbuffer.depth;

	bb.add<DeferredRendererInfo>(info);
}
