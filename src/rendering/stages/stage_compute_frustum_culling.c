#include "stage.h"

static v4 frustum_normalize_plane(v4 p)
{
	return v4_mul_f32(p, 1.f / V3Length(p.xyz));
}

static void compute_frustum_culling_feature(void *self, struct gfx_render_state *rs)
{
	struct frustum_culling_input *context = self;
	struct gfx_command_buffer *cmd = rs->cmd;

	struct gfx_compute_pipeline_def pipeline_def = gfx_compute_pipeline_def_init(&shaders->compute_frustum_culling_program);
	struct gfx_pipeline_st pipeline_st = FetchComputePipeline(&pipeline_def);

	gfx_cmd_bind_bindless(cmd, pipeline_st.bind_point, pipeline_st.layout);
	gfx_cmd_bind_pipeline(cmd, pipeline_st.bind_point, pipeline_st.pipeline);

	m4 proj = pass_context->camera->projection;
	m4 view = pass_context->camera->view;

	m4 proj_t = m4_transpose(proj);

	// TODO: Might not need to transpose.
	v4 frustum_x = frustum_normalize_plane(v4_add(proj_t.c[3], proj_t.c[0]));
	v4 frustum_y = frustum_normalize_plane(v4_add(proj_t.c[3], proj_t.c[1]));
	
	struct {
		m4 view_matrix;

		f32 P00;
		f32 P11;

		f32 z_near;
		f32 z_far;

		f32 frustum[4];

		//f32 lod_base;
		//f32 lod_step;

		//f32 pyramid_width;
		//f32 pyramid_height;

		u32 draw_count;

		/*
		b32 culling_enabled;
		b32 lod_enabled;
		b32 occlusion_enabled;
		b32 distance_check;
		b32 aabb_check;

		f32 aabb_x_min;
		f32 aabb_y_min;
		f32 aabb_z_min;
		f32 aabb_x_max;
		f32 aabb_y_max;
		f32 aabb_z_max;
		*/

		u64 instance_buffer;
		u64 indirect_buffer;
		u64 output_buffer;
	} draw_cull_data;

	draw_cull_data.view_matrix = view;
	
	draw_cull_data.P00 = proj.m00;
	draw_cull_data.P11 = proj.m11;

	draw_cull_data.z_near = pass_context->camera->near_plane;
	draw_cull_data.z_far = pass_context->camera->far_plane;

	draw_cull_data.frustum[0] = frustum_x.x;
	draw_cull_data.frustum[1] = frustum_x.z;
	draw_cull_data.frustum[2] = frustum_y.y;
	draw_cull_data.frustum[3] = frustum_y.z;

	//draw_cull_data.lod_base = 10.f;
	//draw_cull_data.lod_step = 1.5f;

	//draw_cull_data.pyramid_width = 0.f;
	//draw_cull_data.pyramid_height = 0.f;

	draw_cull_data.draw_count = pass_context->mesh_pass->direct_batch_count;

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
	
	draw_cull_data.instance_buffer = pass_context->instance_buffer->device_address;
	draw_cull_data.indirect_buffer = pass_context->indirect_buffer->device_address;
	draw_cull_data.output_buffer   = pass_context->output_buffer->device_address;
	
	gfx_cmd_push_constants(cmd,
			       pipeline_st.layout,
			       VK_SHADER_STAGE_COMPUTE_BIT,
			       sizeof(draw_cull_data), &draw_cull_data);

	gfx_cmd_dispatch(cmd, (draw_cull_data.draw_count / 256) + 1, 1, 1);
}

void stage_add_compute_frustum_culling(struct gfx_render_graph *graph,
				       struct stage_compute_frustum_culling_input *input)
{
	struct gfx_render_stage stage = {0};
	gfx_render_stage_init(&stage, GFX_RENDER_STAGE_compute);

	gfx_render_stage_add_feature(&stage,
				     sizeof(struct stage_compute_frustum_culling_input), input,
				     compute_frustum_culling_feature);
	
	gfx_render_graph_push(graph, &stage);
}
