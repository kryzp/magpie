#include "compute_culling.h"

#include "assets/shader_serializer.h"

#include "math/matrix.h"
#include "math/vec4.h"

#include "../render_scene.h"

using namespace gfx;

void ComputeCulling::init(ast::AssetManager &assets)
{
	compute_frustum_culling_program = assets.get_asset<ast::ShaderAsset>(assets.from_file_path("frustum_culling"))->shader;
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
	struct ComputeCullingStageData {
		RenderResourceHandle object_buffer;
		RenderResourceHandle page_table_buffer;
		RenderResourceHandle mesh_buffer;
	};

	graph.push_stage<ComputeCullingStageData>(
		"Compute Frustum Culling",
		RenderStage::TYPE_COMPUTE,
		[&](RenderGraphBuilder &builder, ComputeCullingStageData &data) -> void {
			data.object_buffer = builder.read_buffer(scene_resources.object_buffer, GPU_BUFFER_ACCESS_COMPUTE_READ_WRITE);
			data.page_table_buffer = builder.read_buffer(scene_resources.page_table_buffer, GPU_BUFFER_ACCESS_COMPUTE_READ_WRITE);
			data.mesh_buffer = builder.read_buffer(graph.import_buffer(scene.get_mesh_buffer()), GPU_BUFFER_ACCESS_COMPUTE_READ_WRITE);

			for (auto &b : scene_resources.indirect_buffers)
				builder.write_buffer(b, GPU_BUFFER_ACCESS_COMPUTE_READ_WRITE);
			
			for (auto &b : scene_resources.counter_buffers)
				builder.write_buffer(b, GPU_BUFFER_ACCESS_COMPUTE_READ_WRITE);
		},
		[=](const RenderContext &ctx, const RenderStageResources &resources, const ComputeCullingStageData &data) -> void {
			CommandBuffer &cmd = ctx.cmd;

			ComputePipelineDef pipeline_def(compute_frustum_culling_program);
			PipelineState pipeline_st = ctx.device.fetch_pipeline(pipeline_def);

			cmd.bind_bindless(pipeline_st.bind_point, pipeline_st.layout, ctx.device.get_bindless());
			cmd.bind_pipeline(pipeline_st.bind_point, pipeline_st.pipeline);

#if 0
			Mat4 proj = ctx.scene_view.camera->get_projection();
			Mat4 view = ctx.scene_view.camera->get_view();

			Mat4 proj_t = proj.transpose();

			// TODO: Might not need to transpose.
			Vec4 frustum_x = (proj_t.c[3] + proj_t.c[0]).frustum_normalize_plane();
			Vec4 frustum_y = (proj_t.c[3] + proj_t.c[1]).frustum_normalize_plane();

			struct {
				Mat4 view_matrix;

				float P00;
				float P11;

				float z_near;
				float z_far;

				float frustum[4];

				/*
				float lod_base;
				float lod_step;

				float pyramid_width;
				float pyramid_height;
				*/

				u32 draw_count;

				/*
				b32 culling_enabled;
				b32 lod_enabled;
				b32 occlusion_enabled;
				b32 distance_check;
				b32 aabb_check;

				float aabb_x_min;
				float aabb_y_min;
				float aabb_z_min;
				float aabb_x_max;
				float aabb_y_max;
				float aabb_z_max;
				*/

				u64 instance_buffer;
				u64 indirect_buffer;
				u64 output_buffer;
			} draw_cull_data;

			draw_cull_data.view_matrix = view;
	
			draw_cull_data.P00 = proj.m00;
			draw_cull_data.P11 = proj.m11;

			draw_cull_data.z_near = ctx.scene_view.camera->get_near();
			draw_cull_data.z_far = ctx.scene_view.camera->get_far();

			draw_cull_data.frustum[0] = frustum_x.x;
			draw_cull_data.frustum[1] = frustum_x.z;
			draw_cull_data.frustum[2] = frustum_y.y;
			draw_cull_data.frustum[3] = frustum_y.z;

			//draw_cull_data.lod_base = 10.f;
			//draw_cull_data.lod_step = 1.5f;

			//draw_cull_data.pyramid_width = 0.f;
			//draw_cull_data.pyramid_height = 0.f;

			draw_cull_data.draw_count = pass.direct_batches.size();

			/*
			draw_cull_data.culling_enabled = true;
			draw_cull_data.lod_enabled = true;
			draw_cull_data.occlusion_enabled = true;
			draw_cull_data.distance_check = true;
			draw_cull_data.aabb_check = true;
	
			// TODO
			draw_cull_data.aabb_x_min = 0.f;
			draw_cull_data.aabb_y_min = 0.f;
			draw_cull_data.aabb_z_min = 0.f;
			draw_cull_data.aabb_x_max = 0.f;
			draw_cull_data.aabb_y_max = 0.f;
			draw_cull_data.aabb_z_max = 0.f;
			*/

			pass.fill_instance_array(*ctx.scene_view.scene, (gpu_types::GpuInstance *)instance_buffer->map());
			pass.fill_indirect_array(*ctx.scene_view.scene, (gpu_types::GpuIndirect *)indirect_buffer->map());
	
			draw_cull_data.instance_buffer = instance_buffer->get_device_address();
			draw_cull_data.indirect_buffer = indirect_buffer->get_device_address();
			draw_cull_data.output_buffer   = output_buffer->get_device_address();

			cmd.push_constants(
				pipeline_st.layout,
				VK_SHADER_STAGE_COMPUTE_BIT,
				sizeof(draw_cull_data), &draw_cull_data
			);
#endif
			
			GpuBufferView object_buffer = resources.get_buffer_view(data.object_buffer);
			GpuBufferView mesh_buffer = resources.get_buffer_view(data.mesh_buffer);
			GpuBufferView page_table_buffer = resources.get_buffer_view(data.page_table_buffer);

			struct {
				u64 object_buffer;
				u64 mesh_buffer;
				u64 page_buffer;
				u32 object_count;
			} pc;

			pc.object_buffer = object_buffer.get_device_address();
			pc.mesh_buffer = mesh_buffer.get_device_address();
			pc.page_buffer = page_table_buffer.get_device_address();
			pc.object_count = ctx.scene.get_object_count();

			cmd.push_constants(
				pipeline_st.layout,
				VK_SHADER_STAGE_COMPUTE_BIT,
				sizeof(pc), &pc
			);

			const u32 threads = 64;

			cmd.dispatch((pc.object_count + threads - 1) / threads, 1, 1);
		}
	);
}
