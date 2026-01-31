#include "deferred_renderer.h"

#include "core/scratch.h"
#include "assets/shader_serializer.h"
#include "math/calc.h"
#include "math/vec3.h"

#include "../render_scene.h"

using namespace gfx;

GFX_BLACKBOARD_DATA(DeferredRendererInfo);

void DeferredRenderer::init(Device *device, ast::AssetManager &assets)
{
	model_shader = assets.get_asset<ast::ShaderAsset>(assets.from_file_path("model"))->shader;

	ambient_lighting_shader      = assets.get_asset<ast::ShaderAsset>(assets.from_file_path("ambient_lighting"))->shader;
	direct_lighting_point_shader = assets.get_asset<ast::ShaderAsset>(assets.from_file_path("direct_lighting_point"))->shader;

	create_light_sphere_mesh(device);
}

// https://songho.ca/opengl/gl_sphere.html
// TODO: Use a more efficient sphere shape like an ICOSPHERE or CUBESPHERE.
void DeferredRenderer::create_light_sphere_mesh(Device *device)
{
	ScratchArena scratch;

	u16 sector_count = 10;
	u16 stack_count  = 10;

	float sector_step = 2.f * CalcF::PI / (float)sector_count;
	float stack_step  =       CalcF::PI / (float)stack_count;

	u32 vertex_count = (stack_count + 1) * (sector_count + 1);
	u32 index_count  = (stack_count - 1) * (sector_count + 0) * 6;

	Vec3 *vertices = scratch.get_arena().push_array<Vec3>(vertex_count);
	u32  *indices  = scratch.get_arena().push_array<u32>(index_count);

	u32 index = 0;

	for (int i = 0; i <= stack_count; i++) {
		float theta = CalcF::PI*0.5f - i*stack_step;
		for (int j = 0; j <= sector_count; j++) {
			float phi = j * sector_step;
			vertices[index++] = Vec3::spherical_to_cartesian(1.f, phi, theta);
		}
	}

	index = 0;

	for (u32 i = 0; i < stack_count; i++) {
		u32 k1 = (sector_count + 1) * (i + 0); // Current stack.
		u32 k2 = (sector_count + 1) * (i + 1); // Next stack.

		for (u32 j = 0; j < sector_count; j++, k1++, k2++) {
			if (i != 0) {
				indices[index + 0] = k1;
				indices[index + 1] = k1 + 1u;
				indices[index + 2] = k2;

				index += 3;
			}

			if (i != stack_count - 1) {
				indices[index + 0] = k1 + 1u;
				indices[index + 1] = k2 + 1u;
				indices[index + 2] = k2;

				index += 3;
			}
		}
	}

	light_sphere_mesh.init(
		device, sizeof(Vec3),
		vertex_count, vertices,
		index_count, indices
	);
}

void DeferredRenderer::destroy()
{
	light_sphere_mesh.destroy();
}

void DeferredRenderer::add_render_stages(
	RenderGraph &graph, RenderGraphBlackboard &bb,
	const RenderScene &scene,
	const GpuBuffer *frame_data,
	RenderResourceHandle irradiance,
	RenderResourceHandle prefilter,
	RenderResourceHandle brdf
)
{
	// TODO: GBuffer should be local to this scope?

	struct GeometryStageData {
		RenderResourceHandle indirect_buffer;
		RenderResourceHandle count_buffer;
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

			for (auto &page : scene.get_geometry_pages()) {
				data.indirect_buffer = builder.read_buffer(graph.import_buffer(page.indirect_buffer), GPU_BUFFER_ACCESS_GRAPHICS_READ_WRITE);
				data.count_buffer = builder.read_buffer(graph.import_buffer(page.draw_count_buffer), GPU_BUFFER_ACCESS_GRAPHICS_READ_WRITE);
			}
		},
		[=](const RenderContext &ctx, const RenderStageResources &resources, const GeometryStageData &data) -> void {
			CommandBuffer &cmd = ctx.cmd;
			
			const GpuBuffer *indirect_buffer = resources.get_buffer(data.indirect_buffer);
			const GpuBuffer *count_buffer = resources.get_buffer(data.count_buffer);

			GraphicsPipelineDef pipeline_def(model_shader);
			pipeline_def.has_depth_attachment = true;

			for (int i = 0; i < GBuffer::ATTACHMENT_MAX_ENUM; i++)
				pipeline_def.colour_attachment_formats.push_back(VK_FORMAT_R32G32B32A32_SFLOAT);

			PipelineState pipeline_st = ctx.device.fetch_pipeline(pipeline_def);

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
			args.object_buffer = ctx.scene_view.scene->get_object_buffer()->get_device_address();
			args.material_buffer = ctx.scene_view.scene->get_material_buffer()->get_device_address();
			args.mesh_buffer = ctx.scene_view.scene->get_mesh_buffer()->get_device_address();
			args.sampler = Sampler::linear->get_bindless_handle();

			cmd.push_constants(
				pipeline_st.layout,
				VK_SHADER_STAGE_ALL_GRAPHICS,
				sizeof(args), &args
			);

			for (auto &page : ctx.scene_view.scene->get_geometry_pages()) {
				cmd.bind_index_buffer(page.index_buffer, 0);
				
				cmd.draw_indexed_indirect_count(
					indirect_buffer, 0,
					count_buffer, 0,
					RenderScene::PAGE_MAX_OBJECTS,
					sizeof(gpu_types::GpuIndirectDraw)
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

			for (int i = 0; i < GBuffer::ATTACHMENT_MAX_ENUM; i++)
				builder.read_texture(gbuffer.attachments[i]);
			
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
			
			pc_ambient.position = resources.get_texture_view(gbuffer.attachments[GBuffer::ATTACHMENT_POSITION])             ->get_bindless_sampled();
			pc_ambient.albedo   = resources.get_texture_view(gbuffer.attachments[GBuffer::ATTACHMENT_ALBEDO])               ->get_bindless_sampled();
			pc_ambient.normal   = resources.get_texture_view(gbuffer.attachments[GBuffer::ATTACHMENT_NORMAL])               ->get_bindless_sampled();
			pc_ambient.material = resources.get_texture_view(gbuffer.attachments[GBuffer::ATTACHMENT_EMISSIVE])             ->get_bindless_sampled();
			pc_ambient.emissive = resources.get_texture_view(gbuffer.attachments[GBuffer::ATTACHMENT_METALLIC_ROUGHNESS])   ->get_bindless_sampled();
			
			pc_ambient.irradiance_map = resources.get_texture_view(data.irradiance)->get_bindless_sampled();
			pc_ambient.prefilter_map = resources.get_texture_view(data.prefilter)->get_bindless_sampled();

			pc_ambient.brdf_lut = resources.get_texture_view(data.brdf)->get_bindless_sampled();

			pc_ambient.linear_sampler = Sampler::linear->get_bindless_handle();

			cmd.push_constants(
				ambient_pipeline_st.layout,
				VK_SHADER_STAGE_ALL_GRAPHICS,
				sizeof(pc_ambient), &pc_ambient
			);

			cmd.draw_vertices_n(3);

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
		
			PipelineState direct_pipeline_st = ctx.device.fetch_pipeline(direct_pipeline_def);

			cmd.bind_bindless(direct_pipeline_st.bind_point, direct_pipeline_st.layout, ctx.device.get_bindless());
			cmd.bind_pipeline(direct_pipeline_st.bind_point, direct_pipeline_st.pipeline);

			light_sphere_mesh.bind_indices(cmd);

			// TODO: Instanced / Indirect Rendering?
			for (int i = 0; i < ctx.scene_view.scene->get_light_count(); i++) {
				struct {
					u64 frame_data_buffer;
					u64 light_buffer;
					u64 vertex_buffer;
					
					u32 position;
					u32 albedo;
					u32 normal;
					u32 emissive;
					u32 material;

					u32 linear_sampler;
				} pc_direct;

				pc_direct.frame_data_buffer = frame_data->get_device_address();
				pc_direct.light_buffer = ctx.scene_view.scene->get_light_buffer()->get_device_address();
				pc_direct.vertex_buffer = light_sphere_mesh.vertex_buffer->get_device_address();
			
				pc_direct.position = resources.get_texture_view(gbuffer.attachments[GBuffer::ATTACHMENT_POSITION])             ->get_bindless_sampled();
				pc_direct.albedo   = resources.get_texture_view(gbuffer.attachments[GBuffer::ATTACHMENT_ALBEDO])               ->get_bindless_sampled();
				pc_direct.normal   = resources.get_texture_view(gbuffer.attachments[GBuffer::ATTACHMENT_NORMAL])               ->get_bindless_sampled();
				pc_direct.emissive = resources.get_texture_view(gbuffer.attachments[GBuffer::ATTACHMENT_EMISSIVE])             ->get_bindless_sampled();
				pc_direct.material = resources.get_texture_view(gbuffer.attachments[GBuffer::ATTACHMENT_METALLIC_ROUGHNESS])   ->get_bindless_sampled();
			
				pc_direct.linear_sampler = Sampler::linear->get_bindless_handle();
				
				cmd.push_constants(
					ambient_pipeline_st.layout,
					VK_SHADER_STAGE_ALL_GRAPHICS,
					sizeof(pc_direct), &pc_direct
				);

				light_sphere_mesh.draw_indexed(cmd, i);
			}
		}
	);

	// ----------------------
	
	DeferredRendererInfo info = {};
	info.gbuffer = gbuffer;

	bb.add<DeferredRendererInfo>(info);
}
