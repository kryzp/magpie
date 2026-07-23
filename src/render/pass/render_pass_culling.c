
internal R_PASS_RECORD_DEF(R_CullClearFn)
{
	const R_CullClearPassData *data = ctx->user_data;
	
	G_ResourceKey counter_key = R_GraphResolveBuffer(ctx->graph, data->counter_handle);
	G_CmdFillBuffer(ctx->cmd, counter_key, 0, G_DeviceBufferSize(counter_key), 0);
}

internal R_PASS_RECORD_DEF(R_CullFrustumComputeFn)
{
	G_CmdBuffer *cmd = ctx->cmd;

	const R_CullPassData *data = ctx->user_data;

	G_ComputePipelineDef pipeline_def = G_ComputePipelineDefInit(data->shader);
	G_PipelineSt pipeline_st = G_DeviceFetchComputePipeline(&pipeline_def);

	R_BufferRange indirect_range = R_GraphResolveBufferRange(ctx->graph, data->indirect_handle);
	R_BufferRange counter_range  = R_GraphResolveBufferRange(ctx->graph, data->counter_handle);

	struct
	{
		u64 object_buffer;
		u64 mesh_buffer;
		u64 material_buffer;
		u64 page_buffer;
		u64 indirect_buffer;
		u64 count_buffer;
		u32 object_count;
		u32 alpha_filter;
		u32 max_draws_per_page;
		u32 _padding1;
		v4 frustum_planes[6];
	}
	pc;

	pc.object_buffer = data->frame_params->object_buffer.gpu;
	pc.mesh_buffer = G_DeviceBufferAddress(data->frame_params->mesh_buffer);
	pc.material_buffer = G_DeviceBufferAddress(data->frame_params->material_buffer);
	pc.page_buffer = data->frame_params->page_table_buffer.gpu;
	pc.indirect_buffer = R_BufferRangeAddress(&indirect_range);
	pc.count_buffer = R_BufferRangeAddress(&counter_range);
	pc.object_count = data->frame_params->object_count;
	pc.alpha_filter = data->filter;
	pc.max_draws_per_page = R_SCENE_MAX_ENTITIES;
	
	for (u32 i = 0; i < 6; i++)
		pc.frustum_planes[i] = data->frustum_planes[i];

	G_CmdBindBindless(cmd, VK_SHADER_STAGE_COMPUTE_BIT, pipeline_st.layout);
	G_CmdBindPipeline(cmd, pipeline_st.bind_point, pipeline_st.pipeline);
	G_CmdPushConstants(cmd, pipeline_st.layout, VK_SHADER_STAGE_COMPUTE_BIT, pc, 0);
	G_CmdDispatch(cmd, G_ComputeGroupCount(pc.object_count, 64), 1, 1);
}

internal R_PASS_RECORD_DEF(R_CullSphereComputeFn)
{
	G_CmdBuffer *cmd = ctx->cmd;

	const R_CullPassData *data = ctx->user_data;

	G_ComputePipelineDef pipeline_def = G_ComputePipelineDefInit(data->shader);
	G_PipelineSt pipeline_st = G_DeviceFetchComputePipeline(&pipeline_def);

	R_BufferRange indirect_range = R_GraphResolveBufferRange(ctx->graph, data->indirect_handle);
	R_BufferRange counter_range = R_GraphResolveBufferRange(ctx->graph, data->counter_handle);

	struct
	{
		u64 object_buffer;
		u64 mesh_buffer;
		u64 material_buffer;
		u64 page_buffer;
		u64 indirect_buffer;
		u64 count_buffer;
		u32 object_count;
		u32 alpha_filter;
		u32 max_draws_per_page;
		u32 _padding1;
		v4 sphere;
	}
	pc;

	pc.object_buffer = data->frame_params->object_buffer.gpu;
	pc.mesh_buffer = G_DeviceBufferAddress(data->frame_params->mesh_buffer);
	pc.material_buffer = G_DeviceBufferAddress(data->frame_params->material_buffer);
	pc.page_buffer = data->frame_params->page_table_buffer.gpu;
	pc.indirect_buffer = R_BufferRangeAddress(&indirect_range);
	pc.count_buffer = R_BufferRangeAddress(&counter_range);
	pc.object_count = data->frame_params->object_count;
	pc.alpha_filter = data->filter;
	pc.max_draws_per_page = R_SCENE_MAX_ENTITIES;
	
	pc.sphere = data->sphere;

	G_CmdBindBindless(cmd, VK_SHADER_STAGE_COMPUTE_BIT, pipeline_st.layout);
	G_CmdBindPipeline(cmd, pipeline_st.bind_point, pipeline_st.pipeline);
	G_CmdPushConstants(cmd, pipeline_st.layout, VK_SHADER_STAGE_COMPUTE_BIT, pc, 0);
	G_CmdDispatch(cmd, G_ComputeGroupCount(pc.object_count, 64), 1, 1);
}

internal R_DrawStream R_CullFrustum(R_Graph *graph,
								  const R_FrameParams *frame_params,
								  R_CullFilter filter,
								  const R_FrustumVolume *frustum)
{
	R_DrawStream stream = {0};

	R_BufferInfo indirect_info = R_BufferInfoInit(R_SCENE_MAX_ENTITIES * sizeof(R_GPU_IndirectDraw) * frame_params->page_count,
												  VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
												  VK_BUFFER_USAGE_2_INDIRECT_BUFFER_BIT |
												  VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT);

	R_BufferInfo counter_info = R_BufferInfoInit(sizeof(u32) * frame_params->page_count,
												 VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
												 VK_BUFFER_USAGE_2_INDIRECT_BUFFER_BIT |
												 VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT |
												 VK_BUFFER_USAGE_2_TRANSFER_DST_BIT);

	R_GraphBufHandle indirect_handle = R_GraphCreateBuffer(graph, &indirect_info);
	R_GraphBufHandle counter_handle = R_GraphCreateBuffer(graph, &counter_info);

	// Clear counter pass.
	{
		R_CullClearPassData *data = ArenaPushArray(frame_params->arena, R_CullClearPassData, 1);
		data->counter_handle = counter_handle;

		R_Pass *pass = R_GraphAdd(graph, String8Lit("Compute Frustum Culling (Clear Counter)"), R_PassType_Transfer);
		R_PassSetRecord(pass, R_CullClearFn, data);
		R_PassClearBuffer(pass, counter_handle);
	}

	// Compute culling pass.
	{
		R_CullPassData *data = ArenaPushArray(frame_params->arena, R_CullPassData, 1);
		data->shader = frame_params->cull_frustum_shader;
		data->indirect_handle = indirect_handle;
		data->counter_handle = counter_handle;
		data->frame_params = frame_params;
		data->filter = filter;
		
		for (u32 i = 0; i < 6; i++)
			data->frustum_planes[i] = frustum->planes[i];

		R_Pass *pass = R_GraphAdd(graph, String8Lit("Compute Frustum Culling"), R_PassType_Compute);
		R_PassSetRecord(pass, R_CullFrustumComputeFn, data);

		stream.indirect_buffer = R_PassWriteBufferCompute(pass, indirect_handle);
		stream.count_buffer = R_PassWriteBufferCompute(pass, counter_handle);
	}

	return stream;
}

internal R_DrawStream R_CullSphere(R_Graph *graph,
								 const R_FrameParams *frame_params,
								 R_CullFilter filter,
								 v3 sphere_centre, f32 sphere_radius)
{
	R_DrawStream stream = {0};

	R_BufferInfo indirect_info = R_BufferInfoInit(R_SCENE_MAX_ENTITIES * sizeof(R_GPU_IndirectDraw) * frame_params->page_count,
												  VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
												  VK_BUFFER_USAGE_2_INDIRECT_BUFFER_BIT |
												  VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT);

	R_BufferInfo counter_info = R_BufferInfoInit(sizeof(u32) * frame_params->page_count,
												 VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
												 VK_BUFFER_USAGE_2_INDIRECT_BUFFER_BIT |
												 VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT |
												 VK_BUFFER_USAGE_2_TRANSFER_DST_BIT);

	R_GraphBufHandle indirect_handle = R_GraphCreateBuffer(graph, &indirect_info);
	R_GraphBufHandle counter_handle = R_GraphCreateBuffer(graph, &counter_info);

	// Clear counter pass.
	{
		R_CullClearPassData *data = ArenaPushArray(frame_params->arena, R_CullClearPassData, 1);
		data->counter_handle = counter_handle;

		R_Pass *pass = R_GraphAdd(graph, String8Lit("Compute Sphere Culling (Clear Counter)"), R_PassType_Transfer);
		R_PassSetRecord(pass, R_CullClearFn, data);
		R_PassClearBuffer(pass, counter_handle);
	}

	// Compute culling pass.
	{
		R_CullPassData *data = ArenaPushArray(frame_params->arena, R_CullPassData, 1);
		data->shader = frame_params->cull_sphere_shader;
		data->indirect_handle = indirect_handle;
		data->counter_handle = counter_handle;
		data->frame_params = frame_params;
		data->filter = filter;
		
		data->sphere = v4(sphere_centre.x, sphere_centre.y, sphere_centre.z, sphere_radius);

		R_Pass *pass = R_GraphAdd(graph, String8Lit("Compute Sphere Culling"), R_PassType_Compute);
		R_PassSetRecord(pass, R_CullSphereComputeFn, data);

		stream.indirect_buffer = R_PassWriteBufferCompute(pass, indirect_handle);
		stream.count_buffer = R_PassWriteBufferCompute(pass, counter_handle);
	}

	return stream;
}
