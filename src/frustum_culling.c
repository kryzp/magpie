
struct frustum_culling_input {
	Camera *camera;
	MeshPass *mesh_pass;
	GPUBuffer *instance_buffer;
	GPUBuffer *indirect_buffer;
	GPUBuffer *output_buffer;
};

internal v4 FrustumNormalizePlane(v4 p)
{
	return V4MultiplyF32(p, 1.f / V3Length(p.xyz));
}

internal void RenderPassComputeFrustumCulling(RenderState *rs, void *context)
{
	CommandBuffer *cmd = &rs->cmd;

	struct frustum_culling_input *pass_context = context;

	ComputePipelineDef pipeline_def = ComputePipelineDefInit(&shaders->compute_frustum_culling_program);
	PipelineState pipeline_st = FetchComputePipeline(&pipeline_def);

	CmdBindBindless(cmd, pipeline_st.bind_point, pipeline_st.layout);
	CmdBindPipeline(cmd, pipeline_st.bind_point, pipeline_st.pipeline);

	m4 proj = pass_context->camera->projection;
	m4 view = pass_context->camera->view;

	m4 proj_t = M4Transpose(proj);

	// TODO: Might not need to transpose.
	v4 frustum_x = FrustumNormalizePlane(V4AddV4(proj_t.c[3], proj_t.c[0]));
	v4 frustum_y = FrustumNormalizePlane(V4AddV4(proj_t.c[3], proj_t.c[1]));
	
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
	
	CmdPushConstants(cmd, pipeline_st.layout, VK_SHADER_STAGE_COMPUTE_BIT, sizeof(draw_cull_data), &draw_cull_data);

	CmdDispatch(cmd, (draw_cull_data.draw_count / 256) + 1, 1, 1);
}

internal void ComputeFrustumCulling(RenderGraph *graph, struct frustum_culling_input *input)
{
	RenderPass compute_pass = {0};
	compute_pass.type = RenderPassType_Compute;
	compute_pass.compute.Record = RenderPassComputeFrustumCulling;
	compute_pass.compute.buffer_count = 3;
	compute_pass.compute.buffers[0] = input->instance_buffer;
	compute_pass.compute.buffers[1] = input->output_buffer;
	compute_pass.compute.buffers[2] = input->indirect_buffer;
	
	MemoryCopy(compute_pass.context, input, sizeof(struct frustum_culling_input));
	
	RenderGraphPush(graph, &compute_pass);
}
