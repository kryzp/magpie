#include "shadow_renderer.h"

#include "math/calc.h"
#include "assets/shader_serializer.h"

using namespace gfx;

void ShadowRenderer::init(Device *device, ast::AssetManager &assets)
{
	this->device = device;

	caster_table_buffer = device->alloc_buffer(
		VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT,
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		MAX_SHADOW_CASTERS * sizeof(gpu_types::GpuShadowCaster)
	);

	for (int i = 0; i < MAX_SHADOW_CASTERS; i++) {
		shadow_cubemaps[i] = device->alloc_texture_cubemap_depth(SHADOW_MAP_RESOLUTION, 1);
		shadow_cubemap_views[i] = device->create_texture_view(shadow_cubemaps[i], VK_IMAGE_VIEW_TYPE_CUBE, SubresourceRange::all_depth());
	}

	depth_shader = assets.get_asset<ast::ShaderAsset>(assets.from_file_path("assets://shadow_mapping.msh"))->shader;
}

void ShadowRenderer::destroy()
{
	device->destroy_buffer(caster_table_buffer);

	for (int i = 0; i < MAX_SHADOW_CASTERS; i++) {
		device->destroy_texture_view(shadow_cubemap_views[i]);
		device->destroy_texture(shadow_cubemaps[i]);
	}
}

void ShadowRenderer::render_shadows(
	RenderGraph &graph, RenderGraphBlackboard &bb,
	const RenderScene &scene,
	const RenderSceneResources &scene_resources,
	ComputeCulling &culling
)
{
	static const Vec3 light_dirs[6] = {
		Vec3( 1.f,  0.f,  0.f), // Right.
		Vec3(-1.f,  0.f,  0.f), // Left.
		Vec3( 0.f,  0.f,  1.f), // Up.
		Vec3( 0.f,  0.f, -1.f), // Down.
		Vec3( 0.f,  1.f,  0.f), // Forward.
		Vec3( 0.f, -1.f,  0.f)  // Backwards.
	};

	static const Vec3 light_ups[6] = {
		Vec3( 0.f,  0.f,  1.f), // Right.
		Vec3( 0.f,  0.f,  1.f), // Left.
		Vec3( 0.f, -1.f,  0.f), // Up.
		Vec3( 0.f,  1.f,  0.f), // Down.
		Vec3( 0.f,  0.f,  1.f), // Forward.
		Vec3( 0.f,  0.f,  1.f)  // Backwards.
	};

	const auto &shadow_casters = scene.get_shadow_casters();
	const u32 caster_count = CalcU::min((u32)shadow_casters.size(), MAX_SHADOW_CASTERS);

	gpu_types::GpuShadowCaster *shadow_caster_gpu_mapping = (gpu_types::GpuShadowCaster *)caster_table_buffer->map();

	for (int i = 0; i < caster_count; i++) {
		const auto &info = shadow_casters[i];
		auto &data = shadow_caster_gpu_mapping[i];

		data.position = info.position;
		data.near_plane = info.near_plane;
		data.far_plane = info.far_plane;
		data.shadow_map = shadow_cubemap_views[i]->get_bindless_handle();

		Mat4 light_proj = Mat4::perspective(90.f, 1.f, info.near_plane, info.far_plane);

		for (int f = 0; f < 6; f++) {
			Mat4 light_view = Mat4::lookat(info.position, info.position + light_dirs[f], light_ups[f]);
			data.face_matrices[f] = light_proj * light_view;
		}
	}

	for (int caster_index = 0; caster_index < caster_count; caster_index++) {
		const auto &info = shadow_casters[caster_index];

		char stage_name[64];
		snprintf(stage_name, sizeof(stage_name), "Shadow Mapping (slot %d)", caster_index);

		RenderStage &shadow_stage = graph.push_stage(stage_name, RenderStage::TYPE_GRAPHICS);
		shadow_stage.set_multi_view_mask(0b111111);

		DrawStream draw_stream = culling.cull_sphere(
			graph, bb, scene, scene_resources,
			info.position, info.radius
		);

		shadow_stage.indirect_buffer(draw_stream.indirect_buffer);
		shadow_stage.indirect_buffer(draw_stream.count_buffer);

		RenderResourceHandle cubemap_rg = graph.import_texture(shadow_cubemaps[caster_index]);

		RenderClear clear(1.f, 0);

		shadow_stage.write_depth(cubemap_rg, SubresourceRange::all_depth(), &clear);

		shadow_stage.set_record([=](const RenderContext &ctx, const RenderStageResources &resources) -> void {
			CommandBuffer &cmd = ctx.cmd;

			GraphicsPipelineDef pipeline_def(depth_shader);
			pipeline_def.has_depth_attachment = true;
			pipeline_def.multi_view_mask = 0b111111;
			pipeline_def.cull_mode = VK_CULL_MODE_FRONT_BIT;

			PipelineState pipeline_st = ctx.cache.fetch_pipeline(pipeline_def);

			cmd.bind_bindless(pipeline_st.bind_point, pipeline_st.layout, ctx.device.get_bindless());
			cmd.bind_pipeline(pipeline_st.bind_point, pipeline_st.pipeline);

			struct {
				u64 object_buffer;
				u64 mesh_buffer;
				u64 caster_data_buffer;
				u32 caster_index;
			} pc;

			pc.object_buffer = scene_resources.object_buffer.gpu;
			pc.mesh_buffer = scene.get_mesh_buffer()->get_device_address();
			pc.caster_data_buffer = caster_table_buffer->get_device_address();
			pc.caster_index = caster_index;

			cmd.push_constants(pipeline_st.layout, VK_SHADER_STAGE_ALL_GRAPHICS, sizeof(pc), &pc);

			auto &pages = scene.get_geometry_pages();

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
	}

	ShadowRendererInfo info = {};
	info.shadow_caster_table = caster_table_buffer;

	bb.add<ShadowRendererInfo>(info);
}
