#include "compute_culling.h"

#include "assets/shader_serializer.h"

#include "math/matrix.h"
#include "math/vec4.h"

#include "../render_scene.h"

using namespace gfx;

void ComputeCulling::init(ast::AssetManager &assets)
{
	ast::ShaderAsset *shader_asset = assets.get_asset<ast::ShaderAsset>(assets.from_file_path("frustum_culling", ast::ASSET_TYPE_SHADER));
	compute_frustum_culling_program = shader_asset->shader;
}

void ComputeCulling::destroy()
{
}

static Vec4 frustum_normalize_plane(const Vec4 &p)
{
	return p / Vec3(p.x, p.y, p.z).length();
}

void ComputeCulling::add_render_stages(RenderGraph &graph, RenderGraphBlackboard &bb, const SceneView &scene_view)
{
	RenderStage &stage = graph.add_stage(RenderStage::TYPE_COMPUTE);

	stage.set_scene_view(scene_view);

	const MeshPass &fwd_pass = scene_view.scene->get_pass(MeshPass::TYPE_FORWARD);
	
	stage.add_buffer(fwd_pass.instance_buffer, sync::GPU_BUFFER_ACCESS_compute_rw);
	stage.add_buffer(fwd_pass.draw_indirect_buffer, sync::GPU_BUFFER_ACCESS_compute_rw);
	stage.add_buffer(fwd_pass.compacted_instance_buffer, sync::GPU_BUFFER_ACCESS_compute_rw);

	stage.set_record([&](const RenderContext &ctx) -> void {
		CommandBuffer &cmd = ctx.cmd;

		MeshPass &pass = ctx.view.scene->get_pass(MeshPass::TYPE_FORWARD);

		ComputePipelineDef pipeline_def(compute_frustum_culling_program);
		PipelineState pipeline_st = graph.get_device().fetch_pipeline(pipeline_def);

		cmd.bind_bindless(pipeline_st.bind_point, pipeline_st.layout, graph.get_device().get_bindless());
		cmd.bind_pipeline(pipeline_st.bind_point, pipeline_st.pipeline);

		Mat4 proj = ctx.view.camera->get_projection();
		Mat4 view = ctx.view.camera->get_view();

		Mat4 proj_t = proj.transpose();

		// TODO: Might not need to transpose.
		Vec4 frustum_x = frustum_normalize_plane(proj_t.c[3] + proj_t.c[0]);
		Vec4 frustum_y = frustum_normalize_plane(proj_t.c[3] + proj_t.c[1]);

		struct {
			Mat4 view_matrix;

			float P00;
			float P11;

			float z_near;
			float z_far;

			float frustum[4];

			//float lod_base;
			//float lod_step;

			//float pyramid_width;
			//float pyramid_height;

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

		draw_cull_data.z_near = ctx.view.camera->get_near();
		draw_cull_data.z_far = ctx.view.camera->get_far();

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
		
		GpuBuffer &instance_buffer = graph.get_physical_buffer(graph.get_resource(pass.instance_buffer));
		GpuBuffer &indirect_buffer = graph.get_physical_buffer(graph.get_resource(pass.draw_indirect_buffer));
		GpuBuffer &output_buffer   = graph.get_physical_buffer(graph.get_resource(pass.compacted_instance_buffer));

		pass.fill_instance_array(*ctx.view.scene, (gpu_types::GpuInstance *)instance_buffer.map());
		pass.fill_indirect_array(*ctx.view.scene, (gpu_types::GpuIndirect *)indirect_buffer.map());
	
		draw_cull_data.instance_buffer = instance_buffer.get_device_address();
		draw_cull_data.indirect_buffer = indirect_buffer.get_device_address();
		draw_cull_data.output_buffer   = output_buffer.get_device_address();

		cmd.push_constants(
			pipeline_st.layout,
			VK_SHADER_STAGE_COMPUTE_BIT,
			sizeof(draw_cull_data), &draw_cull_data
		);

		cmd.dispatch((draw_cull_data.draw_count / 256) + 1, 1, 1);
	});
}
