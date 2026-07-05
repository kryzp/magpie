
static R_PASS_RECORD_DEF(R_CullClearFn)
{
	const R_CullClearPassData *data = ctx->user_data;
	
	G_BufferKey counter_key = R_GraphResolveBuffer(ctx->graph, data->counter_handle);
	G_CmdFillBuffer(ctx->cmd, counter_key, 0, G_DeviceBufferSize(ctx->device, counter_key), 0);
}

static R_PASS_RECORD_DEF(R_CullFrustumComputeFn)
{
	G_Device *device = ctx->device;
	G_CmdBuffer *cmd = ctx->cmd;

	const R_CullPassData *data = ctx->user_data;

	G_ComputePipelineDef pipeline_def = G_ComputePipelineDefInit(data->shader);
	G_PipelineSt pipeline_st = G_DeviceFetchComputePipeline(device, &pipeline_def);

	G_CmdBindBindless(cmd, VK_SHADER_STAGE_COMPUTE_BIT, pipeline_st.layout);
	G_CmdBindPipeline(cmd, pipeline_st.bind_point, pipeline_st.pipeline);

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

	pc.object_buffer   = data->object_buffer_address;
	pc.mesh_buffer     = R_MeshRegistryBufferAddr(&ctx->scene->meshes);
	pc.material_buffer = R_MaterialRegistryBufferAddr(&ctx->scene->materials);
	pc.page_buffer     = data->page_table_buffer_address;

	pc.indirect_buffer = R_BufferRangeAddress(&indirect_range, device);
	pc.count_buffer    = R_BufferRangeAddress(&counter_range, device);
	
	pc.object_count    = R_SceneGraphObjectCount(&ctx->scene->graph);
	
	pc.alpha_filter    = (u32)data->filter;

	pc.max_draws_per_page = R_SCENE_GRAPH_MAX_OBJECTS;
	
	for (u32 i = 0; i < 6; i++)
		pc.frustum_planes[i] = data->frustum_planes[i];

	G_CmdPushConstants (cmd, pipeline_st.layout, VK_SHADER_STAGE_COMPUTE_BIT, sizeof(pc), &pc, 0);
	G_CmdDispatch      (cmd, G_ComputeGroupCount(pc.object_count, 64), 1, 1);
}

static R_PASS_RECORD_DEF(R_CullSphereComputeFn)
{
	G_Device *device = ctx->device;
	G_CmdBuffer *cmd = ctx->cmd;

	const R_CullPassData *data = ctx->user_data;

	G_ComputePipelineDef pipeline_def = G_ComputePipelineDefInit(data->shader);
	G_PipelineSt pipeline_st = G_DeviceFetchComputePipeline(device, &pipeline_def);

	G_CmdBindBindless(cmd, VK_SHADER_STAGE_COMPUTE_BIT, pipeline_st.layout);
	G_CmdBindPipeline(cmd, pipeline_st.bind_point, pipeline_st.pipeline);

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
		v4 sphere;
	}
	pc;

	pc.object_buffer   = data->object_buffer_address;
	pc.mesh_buffer     = R_MeshRegistryBufferAddr(&ctx->scene->meshes);
	pc.material_buffer = R_MaterialRegistryBufferAddr(&ctx->scene->materials);
	pc.page_buffer     = data->page_table_buffer_address;

	pc.indirect_buffer = R_BufferRangeAddress(&indirect_range, device);
	pc.count_buffer    = R_BufferRangeAddress(&counter_range, device);
	
	pc.object_count    = R_SceneGraphObjectCount(&ctx->scene->graph);
	
	pc.alpha_filter    = (u32)data->filter;

	pc.max_draws_per_page = R_SCENE_GRAPH_MAX_OBJECTS;
	
	pc.sphere          = data->sphere;

	G_CmdPushConstants (cmd, pipeline_st.layout, VK_SHADER_STAGE_COMPUTE_BIT, sizeof(pc), &pc, 0);
	G_CmdDispatch      (cmd, G_ComputeGroupCount(pc.object_count, 64), 1, 1);
}

static void R_CullingInit(R_Culling *cull, A_Assets *assets)
{
	cull->assets = assets;
	
	cull->frustum_shader = A_Require(assets, String8Lit("assets://shaders/passes/culling/frustum_culling.slang"), A_Type_Shader);
	cull->sphere_shader  = A_Require(assets, String8Lit("assets://shaders/passes/culling/sphere_culling.slang"),  A_Type_Shader);
}

static void R_CullingDestroy(R_Culling *cull)
{
}

static R_DrawStream R_CullFrustum(R_Culling *cull,
			  R_Graph *graph,
			  const R_Bulletin *bt,
			  R_CullFilter filter,
			  const R_FrustumVolume *frustum)
{
	R_DrawStream stream = {0};

	R_BufferInfo indirect_info = R_BufferInfoInit(R_SCENE_GRAPH_MAX_OBJECTS * sizeof(R_GPU_IndirectDraw) * bt->scene_resources->page_count,
												  VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
												  VK_BUFFER_USAGE_2_INDIRECT_BUFFER_BIT |
												  VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT);

	R_BufferInfo counter_info = R_BufferInfoInit(sizeof(u32) * bt->scene_resources->page_count,
												 VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
												 VK_BUFFER_USAGE_2_INDIRECT_BUFFER_BIT |
												 VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT |
												 VK_BUFFER_USAGE_2_TRANSFER_DST_BIT);

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
		G_ShaderKey shader = A_GetNow(cull->assets, cull->frustum_shader)->shader.key;

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

static R_DrawStream R_CullSphere(R_Culling *cull,
			 R_Graph *graph,
			 const R_Bulletin *bt,
			 R_CullFilter filter,
			 v3 sphere_centre, f32 sphere_radius)
{
	R_DrawStream stream = {0};

	R_BufferInfo indirect_info = R_BufferInfoInit(R_SCENE_GRAPH_MAX_OBJECTS * sizeof(R_GPU_IndirectDraw) * bt->scene_resources->page_count,
												  VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
												  VK_BUFFER_USAGE_2_INDIRECT_BUFFER_BIT |
												  VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT);

	R_BufferInfo counter_info = R_BufferInfoInit(sizeof(u32) * bt->scene_resources->page_count,
												 VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
												 VK_BUFFER_USAGE_2_INDIRECT_BUFFER_BIT |
												 VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT |
												 VK_BUFFER_USAGE_2_TRANSFER_DST_BIT);

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
		G_ShaderKey shader = A_GetNow(cull->assets, cull->sphere_shader)->shader.key;

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
