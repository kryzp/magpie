#include "compute_culling.h"

#include "assets/shader_serializer.h"

#include "../camera.h"
#include "../render_scene.h"

using namespace gfx;

GFX_BLACKBOARD_DATA(ComputeCullingPassData);

void ComputeCulling::init(ast::AssetManager &assets)
{
	compute_frustum_culling_program = assets.get_asset<ast::ShaderAsset>(assets.from_file_path("assets://frustum_culling.msh"))->shader;
}

void ComputeCulling::destroy()
{
}

void ComputeCulling::add_render_stages(
	RenderGraph &graph, RenderGraphBlackboard &bb,
	const RenderScene &scene,
	const RenderSceneResources &scene_resources
)
{
	Vector<RenderResourceHandle> indirect_buffers;
	Vector<RenderResourceHandle> count_buffers;

	RenderStage &compute_stage = graph.push_stage("Compute Frustum Culling", RenderStage::TYPE_COMPUTE);

	RenderResourceHandle mesh_buffer_handle = compute_stage.read_buffer_compute(graph.import_buffer(scene.get_mesh_buffer(), { VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE }));

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

	RenderResourceHandle opaque_indirect_handle = graph.create_buffer(opaque_indirect_info);
	RenderResourceHandle opaque_counter_handle = graph.create_buffer(opaque_counter_info);

	indirect_buffers.push_back(compute_stage.write_buffer_compute(opaque_indirect_handle));
	count_buffers.push_back(compute_stage.write_buffer_compute(opaque_counter_handle));

	compute_stage.set_record([=](const RenderContext &ctx, const RenderStageResources &resources) -> void {
		CommandBuffer &cmd = ctx.cmd;

		gpu_types::GpuPagePointers *mapped_ptrs = scene_resources.page_table_buffer.cpu;

		for (int i = 0; i < indirect_buffers.size(); i++) {
			const GpuBuffer *indirect_buffer = resources.get_buffer(indirect_buffers[i]);
			const GpuBuffer *count_buffer = resources.get_buffer(count_buffers[i]);

			mapped_ptrs[i].opaque_indirect_buffer = indirect_buffer->get_device_address();
			mapped_ptrs[i].opaque_count_buffer = count_buffer->get_device_address();

			cmd.fill_buffer(count_buffer, 0, sizeof(u32), 0); // Reset counter buffer.
		}

		ComputePipelineDef pipeline_def(compute_frustum_culling_program);
		PipelineState pipeline_st = ctx.cache.fetch_pipeline(pipeline_def);

		cmd.bind_bindless(pipeline_st.bind_point, pipeline_st.layout, ctx.device.get_bindless());
		cmd.bind_pipeline(pipeline_st.bind_point, pipeline_st.pipeline);

		struct {
			u64 object_buffer;
			u64 mesh_buffer;
			u64 page_buffer;
			u32 object_count;
			u32 _padding;
			Vec4 frustum_planes[6];
		} pc;

		pc.object_buffer = scene_resources.object_buffer.gpu;
		pc.mesh_buffer = resources.get_buffer_range(mesh_buffer_handle).get_device_address();
		pc.page_buffer = scene_resources.page_table_buffer.gpu;
		pc.object_count = ctx.scene.get_object_count();

		const float aggressiveness = 1.0f;

		Camera camera = ctx.camera;
		camera.set_fov(ctx.camera.get_fov() * aggressiveness);
		camera.recompute();

		Mat4 vpt = (camera.get_projection() * camera.get_view()).transpose();

		pc.frustum_planes[0] = (vpt.c[3] + vpt.c[0]).frustum_normalize_plane(); // left
		pc.frustum_planes[1] = (vpt.c[3] - vpt.c[0]).frustum_normalize_plane(); // right
		pc.frustum_planes[2] = (vpt.c[3] + vpt.c[1]).frustum_normalize_plane(); // bottom
		pc.frustum_planes[3] = (vpt.c[3] - vpt.c[1]).frustum_normalize_plane(); // top
		pc.frustum_planes[4] = (           vpt.c[2]).frustum_normalize_plane(); // near
		pc.frustum_planes[5] = (vpt.c[3] - vpt.c[2]).frustum_normalize_plane(); // far

		cmd.push_constants(
			pipeline_st.layout,
			VK_SHADER_STAGE_COMPUTE_BIT,
			sizeof(pc), &pc
		);

		const u32 threads = 64;

		cmd.dispatch((pc.object_count + threads - 1) / threads, 1, 1);
	});
	
	ComputeCullingPassData pass_data = {};
	pass_data.indirect_buffers = indirect_buffers;
	pass_data.count_buffers = count_buffers;

	bb.add<ComputeCullingPassData>(pass_data);
}
