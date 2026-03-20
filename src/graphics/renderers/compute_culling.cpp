#include "compute_culling.h"

#include "assets/shader_serializer.h"

#include "../camera.h"
#include "../render_scene.h"

using namespace gfx;

void ComputeCulling::init(ast::AssetManager &assets)
{
	this->assets = &assets;

	frustum_culling_asset = assets.from_file_path("assets://frustum_culling.msh");
	sphere_culling_asset = assets.from_file_path("assets://sphere_culling.msh");
}

void ComputeCulling::destroy()
{
}

DrawStream ComputeCulling::cull_frustum(
	RenderGraph &graph, RenderGraphBlackboard &bb,
	const RenderScene &scene,
	const RenderSceneResources &scene_resources,
	const FrustumVolume &frustum
)
{
	DrawStream stream = {};

	GpuBufferInfo opaque_indirect_info(
		RenderScene::PAGE_MAX_OBJECTS * sizeof(gpu_types::GpuIndirectDraw),
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		VK_BUFFER_USAGE_2_INDIRECT_BUFFER_BIT |
		VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT
	);

	GpuBufferInfo opaque_counter_info(
		sizeof(u32),
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		VK_BUFFER_USAGE_2_INDIRECT_BUFFER_BIT |
		VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT |
		VK_BUFFER_USAGE_2_TRANSFER_DST_BIT
	);

	RenderResourceHandle indirect_buffer_handle = graph.create_buffer(opaque_indirect_info);
	RenderResourceHandle counter_buffer_handle = graph.create_buffer(opaque_counter_info);

	RenderStage& clear_counter_stage = graph.push_stage("Compute Frustum Culling (Clear Counter)", RenderStage::TYPE_TRANSFER);

	clear_counter_stage.clear_buffer(counter_buffer_handle);

	clear_counter_stage.set_record([=](const RenderContext &ctx, const RenderStageResources &resources) -> void {
		CommandBuffer &cmd = ctx.cmd;
		cmd.fill_buffer(resources.get_buffer(counter_buffer_handle), 0, sizeof(u32), 0);
	});

	RenderStage& compute_stage = graph.push_stage("Compute Frustum Culling", RenderStage::TYPE_COMPUTE);

	stream.indirect_buffer = compute_stage.write_buffer_compute(indirect_buffer_handle);
	stream.count_buffer = compute_stage.write_buffer_compute(counter_buffer_handle);

	compute_stage.set_record([=](const RenderContext &ctx, const RenderStageResources &resources) -> void {
		CommandBuffer &cmd = ctx.cmd;

		ComputePipelineDef pipeline_def(assets->get_asset<ast::ShaderAsset>(frustum_culling_asset)->shader);
		PipelineState pipeline_st = ctx.cache.fetch_pipeline(pipeline_def);

		cmd.bind_bindless(pipeline_st.bind_point, pipeline_st.layout, ctx.device.get_bindless());
		cmd.bind_pipeline(pipeline_st.bind_point, pipeline_st.pipeline);

		struct {
			u64 object_buffer;
			u64 mesh_buffer;
			u64 page_buffer;

			u64 indirect_buffer;
			u64 count_buffer;

			u32 object_count; u32 _padding;

			Vec4 frustum_planes[6];
		} pc;

		pc.object_buffer = scene_resources.object_buffer.gpu;
		pc.mesh_buffer = scene.get_mesh_buffer()->get_device_address();
		pc.page_buffer = scene_resources.page_table_buffer.gpu;

		pc.indirect_buffer = resources.get_buffer_range(indirect_buffer_handle).get_device_address();
		pc.count_buffer = resources.get_buffer_range(counter_buffer_handle).get_device_address();

		pc.object_count = ctx.scene.get_object_count();

		pc.frustum_planes[0] = frustum.frustum_planes[0];
		pc.frustum_planes[1] = frustum.frustum_planes[1];
		pc.frustum_planes[2] = frustum.frustum_planes[2];
		pc.frustum_planes[3] = frustum.frustum_planes[3];
		pc.frustum_planes[4] = frustum.frustum_planes[4];
		pc.frustum_planes[5] = frustum.frustum_planes[5];

		cmd.push_constants(
			pipeline_st.layout,
			VK_SHADER_STAGE_COMPUTE_BIT,
			sizeof(pc), &pc
		);

		cmd.dispatch(compute_group_count(pc.object_count, 64), 1, 1);
	});

	return stream;
}

DrawStream ComputeCulling::cull_sphere(
	RenderGraph &graph, RenderGraphBlackboard &bb,
	const RenderScene &scene,
	const RenderSceneResources &scene_resources,
	const Vec3 &sphere_centre, float sphere_radius
)
{
	DrawStream stream = {};

	GpuBufferInfo opaque_indirect_info(
		RenderScene::PAGE_MAX_OBJECTS * sizeof(gpu_types::GpuIndirectDraw),
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		VK_BUFFER_USAGE_2_INDIRECT_BUFFER_BIT |
		VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT
	);

	GpuBufferInfo opaque_counter_info(
		sizeof(u32),
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		VK_BUFFER_USAGE_2_INDIRECT_BUFFER_BIT |
		VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT |
		VK_BUFFER_USAGE_2_TRANSFER_DST_BIT
	);

	RenderResourceHandle indirect_buffer_handle = graph.create_buffer(opaque_indirect_info);
	RenderResourceHandle counter_buffer_handle = graph.create_buffer(opaque_counter_info);

	RenderStage &clear_counter_stage = graph.push_stage("Compute Sphere Culling (Clear Counter)", RenderStage::TYPE_TRANSFER);

	clear_counter_stage.clear_buffer(counter_buffer_handle);

	clear_counter_stage.set_record([=](const RenderContext &ctx, const RenderStageResources &resources) -> void {
		CommandBuffer &cmd = ctx.cmd;
		cmd.fill_buffer(resources.get_buffer(counter_buffer_handle), 0, sizeof(u32), 0);
	});

	RenderStage &compute_stage = graph.push_stage("Compute Sphere Culling", RenderStage::TYPE_COMPUTE);

	stream.indirect_buffer = compute_stage.write_buffer_compute(indirect_buffer_handle);
	stream.count_buffer = compute_stage.write_buffer_compute(counter_buffer_handle);

	compute_stage.set_record([=](const RenderContext &ctx, const RenderStageResources &resources) -> void {
		CommandBuffer &cmd = ctx.cmd;

		ComputePipelineDef pipeline_def(assets->get_asset<ast::ShaderAsset>(sphere_culling_asset)->shader);
		PipelineState pipeline_st = ctx.cache.fetch_pipeline(pipeline_def);

		cmd.bind_bindless(pipeline_st.bind_point, pipeline_st.layout, ctx.device.get_bindless());
		cmd.bind_pipeline(pipeline_st.bind_point, pipeline_st.pipeline);

		struct {
			u64 object_buffer;
			u64 mesh_buffer;
			u64 page_buffer;

			u64 indirect_buffer;
			u64 count_buffer;

			u32 object_count; u32 _padding;

			Vec4 sphere;
		} pc;

		pc.object_buffer = scene_resources.object_buffer.gpu;
		pc.mesh_buffer = scene.get_mesh_buffer()->get_device_address();
		pc.page_buffer = scene_resources.page_table_buffer.gpu;

		pc.indirect_buffer = resources.get_buffer_range(indirect_buffer_handle).get_device_address();
		pc.count_buffer = resources.get_buffer_range(counter_buffer_handle).get_device_address();

		pc.object_count = ctx.scene.get_object_count();

		pc.sphere.x = sphere_centre.x;
		pc.sphere.y = sphere_centre.y;
		pc.sphere.z = sphere_centre.z;
		pc.sphere.w = sphere_radius;

		cmd.push_constants(
			pipeline_st.layout,
			VK_SHADER_STAGE_COMPUTE_BIT,
			sizeof(pc), &pc
		);

		const u32 threads = 64;

		cmd.dispatch((pc.object_count + threads - 1) / threads, 1, 1);
	});

	return stream;
}
