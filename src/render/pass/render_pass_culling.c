
R_PASS_RECORD_DEF(R_CullClearFn)
{
	GFX_CmdBuffer *cmd = ctx->cmd;

	const R_CullClearPassData *data = ctx->user_data;
	
	GFX_BufferKey counter_key = R_GraphResolveBuffer(ctx->graph, data->counter_handle);
	GFX_CmdFillBuffer(cmd, counter_key, 0, sizeof(u32), 0);
}

R_PASS_RECORD_DEF(R_CullFrustumComputeFn)
{
	GFX_Device *device = ctx->device;
	GFX_CmdBuffer *cmd = ctx->cmd;

	const R_CullPassData *data = ctx->user_data;

	GFX_ComputePipelineDef pipeline_def = GFX_ComputePipelineDefInit(data->shader);
	GFX_PipelineSt pipeline_st = GFX_DeviceFetchComputePipeline(device, &pipeline_def);

	GFX_CmdBindBindless(cmd, VK_SHADER_STAGE_COMPUTE_BIT, pipeline_st.layout);
	GFX_CmdBindPipeline(cmd, pipeline_st.bind_point, pipeline_st.pipeline);

	R_BufferRange indirect_range = R_GraphResolveBufferRange(ctx->graph, data->indirect_handle);
	R_BufferRange counter_range  = R_GraphResolveBufferRange(ctx->graph, data->counter_handle);

	struct
	{
		u64 object_buffer;
		u64 mesh_buffer;
		u64 page_buffer;

		u64 indirect_buffer;
		u64 count_buffer;

		u32 object_count;
		
		u32 alpha_filter;

		v4 frustum_planes[6];
	}
	pc;

	pc.object_buffer   = data->object_buffer_address;
	pc.mesh_buffer     = R_SceneMeshBufferAddress(ctx->scene);
	pc.page_buffer     = data->page_table_buffer_address;

	pc.indirect_buffer = R_BufferRangeAddress(&indirect_range, device);
	pc.count_buffer    = R_BufferRangeAddress(&counter_range, device);
	
	pc.object_count    = R_SceneGetObjectCount(ctx->scene);
	
	pc.alpha_filter    = (u32)data->filter;

	for (u32 i = 0; i < 6; i++)
		pc.frustum_planes[i] = data->frustum_planes[i];

	GFX_CmdPushConstants (cmd, pipeline_st.layout, VK_SHADER_STAGE_COMPUTE_BIT, sizeof(pc), &pc, 0);
	GFX_CmdDispatch      (cmd, GFX_ComputeGroupCount(pc.object_count, 64), 1, 1);
}

R_PASS_RECORD_DEF(R_CullSphereComputeFn)
{
	GFX_Device *device = ctx->device;
	GFX_CmdBuffer *cmd = ctx->cmd;

	const R_CullPassData *data = ctx->user_data;

	GFX_ComputePipelineDef pipeline_def = GFX_ComputePipelineDefInit(data->shader);
	GFX_PipelineSt pipeline_st = GFX_DeviceFetchComputePipeline(device, &pipeline_def);

	GFX_CmdBindBindless(cmd, VK_SHADER_STAGE_COMPUTE_BIT, pipeline_st.layout);
	GFX_CmdBindPipeline(cmd, pipeline_st.bind_point, pipeline_st.pipeline);

	R_BufferRange indirect_range = R_GraphResolveBufferRange(ctx->graph, data->indirect_handle);
	R_BufferRange counter_range  = R_GraphResolveBufferRange(ctx->graph, data->counter_handle);

	struct
	{
		u64 object_buffer;
		u64 mesh_buffer;
		u64 page_buffer;

		u64 indirect_buffer;
		u64 count_buffer;

		u32 object_count;
		
		u32 alpha_filter;

		v4 sphere;
	}
	pc;

	pc.object_buffer   = data->object_buffer_address;
	pc.mesh_buffer     = R_SceneMeshBufferAddress(ctx->scene);
	pc.page_buffer     = data->page_table_buffer_address;

	pc.indirect_buffer = R_BufferRangeAddress(&indirect_range, device);
	pc.count_buffer    = R_BufferRangeAddress(&counter_range, device);

	pc.object_count    = R_SceneGetObjectCount(ctx->scene);
	
	pc.alpha_filter    = (u32)data->filter;
	
	pc.sphere          = data->sphere;

	GFX_CmdPushConstants (cmd, pipeline_st.layout, VK_SHADER_STAGE_COMPUTE_BIT, sizeof(pc), &pc, 0);
	GFX_CmdDispatch      (cmd, GFX_ComputeGroupCount(pc.object_count, 64), 1, 1);
}

internal void
R_CullingInit(R_Culling *cull, AST_Assets *assets)
{
	cull->assets = assets;
	
	cull->frustum_shader = AST_Require(assets, String8Lit("assets://shaders/passes/culling/frustum_culling.slang"), AST_Type_Shader);
	cull->sphere_shader  = AST_Require(assets, String8Lit("assets://shaders/passes/culling/sphere_culling.slang"),  AST_Type_Shader);
}

internal void
R_CullingDestroy(R_Culling *cull)
{
}

internal R_DrawStream
R_CullFrustum(R_Culling *cull,
			  R_Graph *graph,
			  const R_Bulletin *bt,
			  R_CullFilter filter,
			  const R_FrustumVolume *frustum)
{
	R_DrawStream stream = {0};

	R_BufferInfo indirect_info = R_BufferInfoInit();
	indirect_info.size  = R_SCENE_MAX_OBJECTS * sizeof(R_GPU_IndirectDraw);
	indirect_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	indirect_info.usage = VK_BUFFER_USAGE_2_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT;

	R_BufferInfo counter_info = R_BufferInfoInit();
	counter_info.size  = sizeof(u32);
	counter_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	counter_info.usage = VK_BUFFER_USAGE_2_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_2_TRANSFER_DST_BIT;

	R_GraphBufHandle indirect_handle = R_GraphCreateBuffer(graph, &indirect_info);
	R_GraphBufHandle counter_handle  = R_GraphCreateBuffer(graph, &counter_info);

	// Clear counter pass.
	{
		R_CullClearPassData *data = ArenaPushArray(bt->pass_arena, R_CullClearPassData, 1);
		data->counter_handle = counter_handle;

		R_Pass *pass = R_GraphAdd(graph, String8Lit("Compute Frustum Culling (Clear Counter)"), R_PassType_Transfer);
		R_PassSetRecord(pass, R_CullClearFn, data);
		R_PassClearBuffer(pass, counter_handle);
	}

	// Compute culling pass.
	{
		GFX_ShaderKey shader = AST_AssetShaderGet(AST_GetNow(cull->assets, cull->frustum_shader, AST_Type_Shader));

		R_CullPassData *data = ArenaPushArray(bt->pass_arena, R_CullPassData, 1);
		data->shader                    = shader;
		data->indirect_handle           = indirect_handle;
		data->counter_handle            = counter_handle;
		data->object_buffer_address     = bt->scene_resources->object_buffer.gpu;
		data->page_table_buffer_address = bt->scene_resources->page_table_buffer.gpu;
		data->filter                    = filter;
		
		for (u32 i = 0; i < 6; i++)
			data->frustum_planes[i] = frustum->planes[i];

		R_Pass *pass = R_GraphAdd(graph, String8Lit("Compute Frustum Culling"), R_PassType_Compute);
		R_PassSetRecord(pass, R_CullFrustumComputeFn, data);

		stream.indirect_buffer = R_PassWriteBufferCompute(pass, indirect_handle);
		stream.count_buffer    = R_PassWriteBufferCompute(pass, counter_handle);
	}

	return stream;
}

internal R_DrawStream
R_CullSphere(R_Culling *cull,
			 R_Graph *graph,
			 const R_Bulletin *bt,
			 R_CullFilter filter,
			 v3 sphere_centre, f32 sphere_radius)
{
	R_DrawStream stream = {0};

	R_BufferInfo indirect_info = R_BufferInfoInit();
	indirect_info.size  = R_SCENE_MAX_OBJECTS * sizeof(R_GPU_IndirectDraw);
	indirect_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	indirect_info.usage = VK_BUFFER_USAGE_2_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT;

	R_BufferInfo counter_info = R_BufferInfoInit();
	counter_info.size  = sizeof(u32);
	counter_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	counter_info.usage = VK_BUFFER_USAGE_2_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_2_TRANSFER_DST_BIT;

	R_GraphBufHandle indirect_handle = R_GraphCreateBuffer(graph, &indirect_info);
	R_GraphBufHandle counter_handle  = R_GraphCreateBuffer(graph, &counter_info);

	// Clear counter pass.
	{
		R_CullClearPassData *data = ArenaPushArray(bt->pass_arena, R_CullClearPassData, 1);
		data->counter_handle = counter_handle;

		R_Pass *pass = R_GraphAdd(graph, String8Lit("Compute Sphere Culling (Clear Counter)"), R_PassType_Transfer);
		R_PassSetRecord(pass, R_CullClearFn, data);
		R_PassClearBuffer(pass, counter_handle);
	}

	// Compute culling pass.
	{
		GFX_ShaderKey shader = AST_AssetShaderGet(AST_GetNow(cull->assets, cull->sphere_shader, AST_Type_Shader));

		R_CullPassData *data = ArenaPushArray(bt->pass_arena, R_CullPassData, 1);
		data->shader                    = shader;
		data->indirect_handle           = indirect_handle;
		data->counter_handle            = counter_handle;
		data->object_buffer_address     = bt->scene_resources->object_buffer.gpu;
		data->page_table_buffer_address = bt->scene_resources->page_table_buffer.gpu;
		data->filter                    = filter;
		
		data->sphere = v4(sphere_centre.x, sphere_centre.y, sphere_centre.z, sphere_radius);

		R_Pass *pass = R_GraphAdd(graph, String8Lit("Compute Sphere Culling"), R_PassType_Compute);
		R_PassSetRecord(pass, R_CullSphereComputeFn, data);

		stream.indirect_buffer = R_PassWriteBufferCompute(pass, indirect_handle);
		stream.count_buffer    = R_PassWriteBufferCompute(pass, counter_handle);
	}

	return stream;
}
