#include "deferred_renderer.h"

#include "core/scratch.h"
#include "assets/shader_serializer.h"
#include "math/calc.h"
#include "math/vec3.h"

#include "../render_scene.h"

#include "shadow_renderer.h"

using namespace gfx;

void DeferredRenderer::init(Device *device, ast::AssetManager &assets)
{
	model_shader = assets.get_asset<ast::ShaderAsset>(assets.from_file_path("assets://model.msh"))->shader;

	ambient_lighting_shader      = assets.get_asset<ast::ShaderAsset>(assets.from_file_path("assets://ambient_lighting.msh"))->shader;
	direct_lighting_point_shader = assets.get_asset<ast::ShaderAsset>(assets.from_file_path("assets://direct_lighting_point.msh"))->shader;

	create_light_sphere_mesh(device);
}

// https://songho.ca/opengl/gl_sphere.html
// TODO: Use a more efficient sphere shape like an ICOSPHERE or CUBESPHERE.
void DeferredRenderer::create_light_sphere_mesh(Device *device)
{
	ScratchScope scratch = scratch::get();

	u16 sector_count = 10;
	u16 stack_count  = 10;

	float sector_step = 2.f * CalcF::PI / (float)sector_count;
	float stack_step  =       CalcF::PI / (float)stack_count;

	u32 vertex_count = (stack_count + 1) * (sector_count + 1);
	u32 index_count  = (stack_count - 1) * (sector_count + 0) * 6;

	Vec3      *vertices = scratch.arena().array<Vec3>(vertex_count);
	IndexType *indices  = scratch.arena().array<IndexType>(index_count);

	u32 index = 0;

	for (int i = 0; i <= stack_count; i++) {
		float theta = CalcF::PI*0.5f - i*stack_step;
		for (int j = 0; j <= sector_count; j++) {
			float phi = j * sector_step;
			vertices[index++] = Vec3::spherical_to_cartesian(1.f, phi, theta);
		}
	}

	index = 0;

	for (IndexType i = 0; i < stack_count; i++) {
		IndexType k1 = (sector_count + 1) * (i + 0); // Current stack.
		IndexType k2 = (sector_count + 1) * (i + 1); // Next stack.

		for (IndexType j = 0; j < sector_count; j++, k1++, k2++) {
			if (i != 0) {
				indices[index + 0] = k2;
				indices[index + 1] = k1 + 1u;
				indices[index + 2] = k1;

				index += 3;
			}

			if (i != stack_count - 1) {
				indices[index + 0] = k2;
				indices[index + 1] = k2 + 1u;
				indices[index + 2] = k1 + 1u;

				index += 3;
			}
		}
	}

	light_sphere_mesh.create_buffers(
		device, sizeof(Vec3),
		vertex_count, index_count
	);
	
	GpuBuffer *staging_buffer = device->alloc_stage(
		light_sphere_mesh.get_vertex_buffer_size() + light_sphere_mesh.get_index_buffer_size()
	);
	
	light_sphere_mesh.write_to_staging_buffer(staging_buffer, 0, vertices, indices);

	device->submit_graphics_immediate([&](CommandBuffer &cmd) {
		light_sphere_mesh.batch_upload(cmd, staging_buffer, 0);
	});

	device->destroy_buffer(staging_buffer);
}

void DeferredRenderer::destroy()
{
	light_sphere_mesh.destroy_buffers();
}

GBuffer DeferredRenderer::render_geometry(
	RenderGraph &graph, RenderGraphBlackboard &bb,
	const RenderSceneResources &scene_resources,
	const GpuBuffer *frame_data,
	const DrawStream &draw_stream
)
{
	GBuffer gbuffer = {};

	RenderClear depth_clear(1.f, 0);
	AttachmentInfo depth_info(graph.get_device().get_context().get_depth_format());

	RenderClear colour_clear(0.f, 0.f, 0.f, 1.f);
	AttachmentInfo attachment_info(VK_FORMAT_R32G32B32A32_SFLOAT);

	RenderStage &geometry_stage = graph.push_stage("Geometry", RenderStage::TYPE_GRAPHICS);

	for (int i = 0; i < GBuffer::ATTACHMENT_MAX_ENUM; i++) {
		gbuffer.attachments[i] = graph.create_texture(attachment_info);
		geometry_stage.write_colour(gbuffer.attachments[i], SubresourceRange::all_colour(), &colour_clear);
	}

	gbuffer.depth = graph.create_texture(depth_info);

	geometry_stage.write_depth(gbuffer.depth, SubresourceRange::all_depth(), &depth_clear);

	geometry_stage.indirect_buffer(draw_stream.indirect_buffer);
	geometry_stage.indirect_buffer(draw_stream.count_buffer);

	geometry_stage.set_record([=](const RenderContext &ctx, const RenderStageResources &resources) -> void {
		CommandBuffer &cmd = ctx.cmd;

		GraphicsPipelineDef pipeline_def(model_shader);
		pipeline_def.has_depth_attachment = true;

		for (int i = 0; i < GBuffer::ATTACHMENT_MAX_ENUM; i++)
			pipeline_def.colour_attachment_formats.push_back(VK_FORMAT_R32G32B32A32_SFLOAT);

		PipelineState pipeline_st = ctx.cache.fetch_pipeline(pipeline_def);

		cmd.bind_bindless(pipeline_st.bind_point, pipeline_st.layout, ctx.device.get_bindless());
		cmd.bind_pipeline(pipeline_st.bind_point, pipeline_st.pipeline);

		struct {
			u64 frame_data_buffer;
			u64 object_buffer;
			u64 material_buffer;
			u64 mesh_buffer;
			u32 sampler;
		} args;

		args.frame_data_buffer = frame_data->get_device_address();
		args.object_buffer = scene_resources.object_buffer.gpu;
		args.material_buffer = ctx.scene.get_material_buffer()->get_device_address();
		args.mesh_buffer = ctx.scene.get_mesh_buffer()->get_device_address();
		args.sampler = Sampler::linear->get_bindless_handle();

		cmd.push_constants(
			pipeline_st.layout,
			VK_SHADER_STAGE_ALL_GRAPHICS,
			sizeof(args), &args
		);

		auto &pages = ctx.scene.get_geometry_pages();

		const GpuBuffer *indirect_buffer = resources.get_buffer(draw_stream.indirect_buffer);
		const GpuBuffer *counter_buffer = resources.get_buffer(draw_stream.count_buffer);

		for (auto &page : pages) {
			cmd.bind_index_buffer(page.index_buffer, 0);

			cmd.draw_indexed_indirect_count(
				indirect_buffer, 0,
				counter_buffer, 0,
				RenderScene::PAGE_MAX_OBJECTS,
				sizeof(gpu_types::GpuIndirectDraw)
			);
		}
	});

	return gbuffer;
}

RenderResourceHandle DeferredRenderer::render_lighting(
	RenderGraph &graph, RenderGraphBlackboard &bb,
	const RenderSceneResources &scene_resources,
	const GBuffer &gbuffer,
	const GpuBuffer *frame_data,
	const DrawStream &draw_stream,
	const Texture *irradiance,
	const Texture *prefilter,
	const Texture *brdf
)
{
	auto &shadow_info = bb.get<ShadowRendererInfo>();

	RenderStage &lighting_stage = graph.push_stage("Lighting", RenderStage::TYPE_GRAPHICS);
	
	AttachmentInfo lighting_info(VK_FORMAT_R32G32B32A32_SFLOAT);
	RenderResourceHandle lighting = graph.create_texture(lighting_info);

	lighting_stage.write_colour(lighting);
	lighting_stage.write_depth(gbuffer.depth);

	for (int i = 0; i < GBuffer::ATTACHMENT_MAX_ENUM; i++)
		lighting_stage.read_texture(gbuffer.attachments[i]);

	RenderResourceHandle lighting_irradiance_handle = lighting_stage.read_texture(graph.import_texture(irradiance));
	RenderResourceHandle lighting_prefilter_handle = lighting_stage.read_texture(graph.import_texture(prefilter));
	RenderResourceHandle lighting_brdf_handle = lighting_stage.read_texture(graph.import_texture(brdf));

	lighting_stage.set_record([=](const RenderContext &ctx, const RenderStageResources &resources) -> void {
		CommandBuffer &cmd = ctx.cmd;
		
		// --- AMBIENT
		GraphicsPipelineDef ambient_pipeline_def(ambient_lighting_shader);
		ambient_pipeline_def.has_depth_attachment = true;
		ambient_pipeline_def.depth_stencil_state.depth_test_enabled = false;
		ambient_pipeline_def.depth_stencil_state.depth_write_enabled = false;
		ambient_pipeline_def.colour_attachment_formats = { VK_FORMAT_R32G32B32A32_SFLOAT };

		PipelineState ambient_pipeline_st = ctx.cache.fetch_pipeline(ambient_pipeline_def);

		cmd.bind_bindless(ambient_pipeline_st.bind_point, ambient_pipeline_st.layout, ctx.device.get_bindless());
		cmd.bind_pipeline(ambient_pipeline_st.bind_point, ambient_pipeline_st.pipeline);

		struct {
			u64 frame_data_buffer;
		
			u32 position;
			u32 albedo;
			u32 normal;
			u32 emissive;
			u32 material;

			u32 irradiance_map;
			u32 prefilter_map;

			u32 brdf_lut;
			
			u32 linear_sampler;
		} pc_ambient;

		pc_ambient.frame_data_buffer = frame_data->get_device_address();
			
		pc_ambient.position = resources.get_texture_view(gbuffer.attachments[GBuffer::ATTACHMENT_POSITION], SubresourceRange::all_colour())             ->get_bindless_handle();
		pc_ambient.albedo   = resources.get_texture_view(gbuffer.attachments[GBuffer::ATTACHMENT_ALBEDO], SubresourceRange::all_colour())               ->get_bindless_handle();
		pc_ambient.normal   = resources.get_texture_view(gbuffer.attachments[GBuffer::ATTACHMENT_NORMAL], SubresourceRange::all_colour())               ->get_bindless_handle();
		pc_ambient.emissive = resources.get_texture_view(gbuffer.attachments[GBuffer::ATTACHMENT_EMISSIVE], SubresourceRange::all_colour())             ->get_bindless_handle();
		pc_ambient.material = resources.get_texture_view(gbuffer.attachments[GBuffer::ATTACHMENT_METALLIC_ROUGHNESS], SubresourceRange::all_colour())   ->get_bindless_handle();

		pc_ambient.irradiance_map = resources.get_texture_view(lighting_irradiance_handle, SubresourceRange::all_colour())->get_bindless_handle();
		pc_ambient.prefilter_map = resources.get_texture_view(lighting_prefilter_handle, SubresourceRange::all_colour())->get_bindless_handle();
		pc_ambient.brdf_lut = resources.get_texture_view(lighting_brdf_handle, SubresourceRange::all_colour())->get_bindless_handle();

		pc_ambient.linear_sampler = Sampler::linear->get_bindless_handle();

		cmd.push_constants(
			ambient_pipeline_st.layout,
			VK_SHADER_STAGE_ALL_GRAPHICS,
			sizeof(pc_ambient), &pc_ambient
		);

		cmd.draw(3);

		// --- DIRECT
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
		
		PipelineState direct_pipeline_st = ctx.cache.fetch_pipeline(direct_pipeline_def);

		cmd.bind_bindless(direct_pipeline_st.bind_point, direct_pipeline_st.layout, ctx.device.get_bindless());
		cmd.bind_pipeline(direct_pipeline_st.bind_point, direct_pipeline_st.pipeline);

		light_sphere_mesh.bind_indices(cmd);

		// TODO: Instanced / Indirect Rendering?
		for (int i = 0; i < ctx.scene.get_light_count(); i++) {
			struct {
				u64 frame_data_buffer;
				u64 light_buffer;
				u64 vertex_buffer;
				u64 shadow_caster_buffer;

				u32 position;
				u32 albedo;
				u32 normal;
				u32 emissive;
				u32 material;

				u32 linear_sampler;
				u32 shadow_sampler;
			} pc_direct;

			pc_direct.frame_data_buffer = frame_data->get_device_address();
			pc_direct.light_buffer = scene_resources.light_buffer.gpu;
			pc_direct.vertex_buffer = light_sphere_mesh.vertex_buffer->get_device_address();
			pc_direct.shadow_caster_buffer = shadow_info.shadow_caster_table->get_device_address();

			pc_direct.position = resources.get_texture_view(gbuffer.attachments[GBuffer::ATTACHMENT_POSITION], SubresourceRange::all_colour())             ->get_bindless_handle();
			pc_direct.albedo   = resources.get_texture_view(gbuffer.attachments[GBuffer::ATTACHMENT_ALBEDO], SubresourceRange::all_colour())               ->get_bindless_handle();
			pc_direct.normal   = resources.get_texture_view(gbuffer.attachments[GBuffer::ATTACHMENT_NORMAL], SubresourceRange::all_colour())               ->get_bindless_handle();
			pc_direct.emissive = resources.get_texture_view(gbuffer.attachments[GBuffer::ATTACHMENT_EMISSIVE], SubresourceRange::all_colour())             ->get_bindless_handle();
			pc_direct.material = resources.get_texture_view(gbuffer.attachments[GBuffer::ATTACHMENT_METALLIC_ROUGHNESS], SubresourceRange::all_colour())   ->get_bindless_handle();
			
			pc_direct.linear_sampler = Sampler::linear->get_bindless_handle();
			pc_direct.shadow_sampler = Sampler::nearest->get_bindless_handle();

			cmd.push_constants(
				direct_pipeline_st.layout,
				VK_SHADER_STAGE_ALL_GRAPHICS,
				sizeof(pc_direct), &pc_direct
			);

			light_sphere_mesh.draw_indexed(cmd, i);
		}
	});

	return lighting;
}
