#include "deferred_renderer.h"

#include "assets/shader_serializer.h"

#include "../render_scene.h"

using namespace gfx;

GFX_IMPLEMENT_BLACKBOARD_DATA(DeferredRendererInfo);

void DeferredRenderer::init(RenderGraph &graph, ast::AssetManager &assets)
{
	this->device = &graph.get_device();

	linear_sampler = device->create_sampler(VK_FILTER_LINEAR);

	ast::ShaderAsset *model_shader_asset = assets.get_asset<ast::ShaderAsset>(assets.from_file_path("model"));
	model_shader = model_shader_asset->shader;

	ast::ShaderAsset *ambient_lighting_shader_asset = assets.get_asset<ast::ShaderAsset>(assets.from_file_path("ambient_lighting"));
	ambient_lighting_shader = ambient_lighting_shader_asset->shader;

	ast::ShaderAsset *direct_lighting_point_shader_asset = assets.get_asset<ast::ShaderAsset>(assets.from_file_path("direct_lighting_point"));
	direct_lighting_point_shader = direct_lighting_point_shader_asset->shader;

	for (int i = 0; i < GBuffer::ATTACHMENT_MAX_ENUM; i++) {
		AttachmentInfo attachment_info(VK_FORMAT_R32G32B32A32_SFLOAT);
		gbuffer.attachments[i] = graph.create_texture_resource(attachment_info);
	}
	
	AttachmentInfo depth_info(device->get_depth_format());
	gbuffer.depth = graph.create_texture_resource(depth_info);
}

void DeferredRenderer::destroy()
{
	device->destroy_sampler(linear_sampler);
}

void DeferredRenderer::add_render_stages(
	RenderGraph &graph, RenderGraphBlackboard &bb,
	const SceneView &scene_view,
	const GpuBuffer &frame_data,
	const EnvironmentProbe &probe, const RenderResourceHandle &brdf
)
{
	DeferredRendererInfo info = {};
	info.lighting = gbuffer.attachments[GBuffer::ATTACHMENT_LIGHTING];
	info.depth = gbuffer.depth;
	bb.add<DeferredRendererInfo>(info);

	RenderStage &geometry_stage = graph.add_stage(RenderStage::TYPE_GRAPHICS);

	geometry_stage.set_scene_view(scene_view);

	for (int i = 0; i < GBuffer::ATTACHMENT_MAX_ENUM; i++) {
		RenderClear clear(0.f, 0.f, 0.f, 1.f);
		geometry_stage.add_colour_output(gbuffer.attachments[i], &clear);
	}

	RenderClear depth_clear(1.f, 0);
	geometry_stage.set_depth_stencil(gbuffer.depth, &depth_clear);
	
	const MeshPass &fwd_pass = scene_view.scene->get_pass(MeshPass::TYPE_FORWARD);
	
	geometry_stage.add_buffer(fwd_pass.draw_indirect_buffer, sync::GPU_BUFFER_ACCESS_indirect);
	geometry_stage.add_buffer(fwd_pass.compacted_instance_buffer, sync::GPU_BUFFER_ACCESS_graphics_rw);

	geometry_stage.set_record([&, frame_data](const RenderContext &ctx) -> void {
		CommandBuffer &cmd = ctx.cmd;

		GraphicsPipelineDef pipeline_def(model_shader);
		pipeline_def.has_depth_attachment = true;

		for (int i = 0; i < GBuffer::ATTACHMENT_MAX_ENUM; i++)
			pipeline_def.colour_attachment_formats.push_back(VK_FORMAT_R32G32B32A32_SFLOAT);

		PipelineState pipeline_st = device->fetch_pipeline(pipeline_def);

		cmd.bind_bindless(pipeline_st.bind_point, pipeline_st.layout, device->get_bindless());
		cmd.bind_pipeline(pipeline_st.bind_point, pipeline_st.pipeline);

		MeshPass &pass = ctx.view.scene->get_pass(MeshPass::TYPE_FORWARD);

		for (auto &mb : pass.multi_batches) {
			const MeshPass::IndirectBatch &b = pass.batches[mb.first];
			
			RenderMesh *mesh = ctx.view.scene->get_mesh(b.mesh_id);
			mesh->original->bind_indices(cmd);
			
			RenderResource instance_buffer_resource = graph.get_resource(pass.compacted_instance_buffer);
			RenderResource indirect_buffer_resource = graph.get_resource(pass.draw_indirect_buffer);

			GpuBuffer &instance_buffer = graph.get_physical_buffer(instance_buffer_resource);
			GpuBuffer &indirect_buffer = graph.get_physical_buffer(indirect_buffer_resource);

			struct {
				u64 frame_data_buffer;
				u64 transform_buffer;
				u64 material_buffer;
				u64 vertex_buffer;
				u64 instance_buffer;
				u32 material_id;
				u32 sampler;
			} args;
			
			args.frame_data_buffer = frame_data.get_device_address();
			args.transform_buffer = ctx.view.scene->get_object_buffer().get_device_address();
			args.material_buffer = ctx.view.scene->get_material_buffer().get_device_address();
			args.vertex_buffer = mesh->original->vertex_buffer.get_device_address();
			args.instance_buffer = instance_buffer.get_device_address();
			args.material_id = b.material_id;
			args.sampler = linear_sampler.get_bindless().sampler;

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
	});

	// ----------------------
	
	RenderStage &lighting_stage = graph.add_stage(RenderStage::TYPE_GRAPHICS);
	
	lighting_stage.set_scene_view(scene_view);

	lighting_stage.add_colour_output(gbuffer.attachments[GBuffer::ATTACHMENT_LIGHTING]);
	lighting_stage.set_depth_stencil(gbuffer.depth);

	for (int i = 0; i < GBuffer::ATTACHMENT_MAX_ENUM; i++)
		lighting_stage.add_texture(gbuffer.attachments[i], sync::TEXTURE_ACCESS_graphics_r);

	// TODO: add irradiance + prefilter.

	lighting_stage.set_record([&, frame_data, probe, brdf](const RenderContext &ctx) -> void {
		CommandBuffer &cmd = ctx.cmd;
		
		// --- AMBIENT
		GraphicsPipelineDef ambient_pipeline_def(ambient_lighting_shader);
		ambient_pipeline_def.has_depth_attachment = true;
		ambient_pipeline_def.depth_stencil_state.depth_test_enabled = false;
		ambient_pipeline_def.depth_stencil_state.depth_write_enabled = false;
		ambient_pipeline_def.colour_attachment_formats = { VK_FORMAT_R32G32B32A32_SFLOAT };

		PipelineState ambient_pipeline_st = device->fetch_pipeline(ambient_pipeline_def);

		cmd.bind_bindless(ambient_pipeline_st.bind_point, ambient_pipeline_st.layout, device->get_bindless());
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

		pc_ambient.frame_data_buffer = frame_data.get_device_address();

		pc_ambient.position = graph.get_physical_texture_view(graph.get_resource(gbuffer.attachments[GBuffer::ATTACHMENT_POSITION])).get_bindless().sampled;
		pc_ambient.albedo   = graph.get_physical_texture_view(graph.get_resource(gbuffer.attachments[GBuffer::ATTACHMENT_ALBEDO])).get_bindless().sampled;
		pc_ambient.normal   = graph.get_physical_texture_view(graph.get_resource(gbuffer.attachments[GBuffer::ATTACHMENT_NORMAL])).get_bindless().sampled;
		pc_ambient.material = graph.get_physical_texture_view(graph.get_resource(gbuffer.attachments[GBuffer::ATTACHMENT_EMISSIVE])).get_bindless().sampled;
		pc_ambient.emissive = graph.get_physical_texture_view(graph.get_resource(gbuffer.attachments[GBuffer::ATTACHMENT_METALLIC_ROUGHNESS])).get_bindless().sampled;
		
		pc_ambient.irradiance_map = 0;//graph.get_physical_texture_view(graph.get_resource(probe.irradiance)).get_bindless().sampled;
		pc_ambient.prefilter_map = 0;//graph.get_physical_texture_view(graph.get_resource(probe.prefilter)).get_bindless().sampled;
		pc_ambient.brdf_lut = 0;//graph.get_physical_texture_view(graph.get_resource(brdf)).get_bindless().sampled;
		
		pc_ambient.linear_sampler = 0;

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
		
		PipelineState direct_pipeline_st = graph.get_device().fetch_pipeline(direct_pipeline_def);

		cmd.bind_bindless(direct_pipeline_st.bind_point, direct_pipeline_st.layout, graph.get_device().get_bindless());
		cmd.bind_pipeline(direct_pipeline_st.bind_point, direct_pipeline_st.pipeline);
		*/
	});
}
