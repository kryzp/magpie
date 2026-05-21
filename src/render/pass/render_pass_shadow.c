
R_PASS_RECORD_DEF(R_ShadowMappingPassFn)
{
	GFX_Device *device = ctx->device;
	GFX_CmdBuffer *cmd = ctx->cmd;

	const R_ShadowMappingPassData *data = ctx->user_data;

	GFX_GraphicsPipelineDef pipeline_def = GFX_GraphicsPipelineDefFromInfo(data->shader, ctx->render_info);
	pipeline_def.depth_stencil_state.depth_test_enabled = true;
	pipeline_def.depth_stencil_state.depth_write_enabled = true;

	GFX_PipelineSt pipeline_st = GFX_DeviceFetchGraphicsPipeline(device, &pipeline_def);

	GFX_CmdBindBindless (cmd, VK_SHADER_STAGE_ALL_GRAPHICS, pipeline_st.layout);
	GFX_CmdBindPipeline (cmd, pipeline_st.bind_point, pipeline_st.pipeline);

	struct
	{
		u64 object_buffer;
		u64 mesh_buffer;
		u64 caster_data_buffer;
		u32 caster_index;
	}
	pc;

	pc.object_buffer      = data->object_buffer_address;
	pc.mesh_buffer        = R_SceneMeshBufferAddr(ctx->scene);
	pc.caster_data_buffer = GFX_DeviceBufferAddress(device, data->caster_table_buffer);
	pc.caster_index       = data->caster_index;

	GFX_CmdPushConstants(cmd, pipeline_st.layout, VK_SHADER_STAGE_ALL_GRAPHICS, sizeof(pc), &pc, 0);

	GFX_BufferKey indirect_key = R_GraphResolveBuffer(ctx->graph, data->draw_stream.indirect_buffer);
	GFX_BufferKey counter_key  = R_GraphResolveBuffer(ctx->graph, data->draw_stream.count_buffer);

	R_SceneDrawIndirect(ctx->scene, cmd, indirect_key, counter_key);
}

internal void
R_ShadowRendererInit(R_ShadowRenderer *sr, GFX_Device *device, AST_Assets *assets)
{
	sr->device = device;
	sr->assets = assets;

	GFX_BufferAllocInfo caster_buf_info = {0};
	caster_buf_info.usage = VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT;
	caster_buf_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	caster_buf_info.size  = R_SCENE_MAX_SHADOW_CASTERS * sizeof(R_GPU_ShadowCaster);

	sr->caster_table_buffer = GFX_DeviceBufferAlloc(device, &caster_buf_info);

	for (u32 i = 0; i < R_SCENE_MAX_SHADOW_CASTERS; i++)
	{
		sr->shadow_cubemaps[i] = GFX_DeviceTextureAllocCubemapDepth(device, R_SHADOW_MAP_RESOLUTION, 1);

		GFX_TextureViewCreateInfo view_info = {0};
		view_info.texture = sr->shadow_cubemaps[i];
		view_info.type = VK_IMAGE_VIEW_TYPE_CUBE;
		view_info.range = GFX_SubresourceRangeAllDepth();

		sr->shadow_cubemap_views[i] = GFX_DeviceTextureViewFetch(device, &view_info);
	}

	sr->depth_shader = AST_Require(assets, String8Lit("assets://shaders/passes/shadow/shadow_mapping.slang"), AST_Type_Shader);
}

internal void
R_ShadowRendererDestroy(R_ShadowRenderer *sr)
{
	GFX_DeviceBufferDestroy(sr->device, sr->caster_table_buffer);

	for (u32 i = 0; i < R_SCENE_MAX_SHADOW_CASTERS; i++)
		GFX_DeviceTextureDestroy(sr->device, sr->shadow_cubemaps[i]);
}

internal void
R_ShadowRendererUploadGPU(R_ShadowRenderer *sr,
						  const R_Scene *scene)
{
	static const v3 light_dirs[6] = {
		{  1.f,  0.f,  0.f }, // Right.
		{ -1.f,  0.f,  0.f }, // Left.
		{  0.f,  0.f,  1.f }, // Up.
		{  0.f,  0.f, -1.f }, // Down.
		{  0.f,  1.f,  0.f }, // Forward.
		{  0.f, -1.f,  0.f }, // Backwards.
	};

	static const v3 light_ups[6] = {
		{  0.f,  0.f,  1.f }, // Right.
		{  0.f,  0.f,  1.f }, // Left.
		{  0.f, -1.f,  0.f }, // Up.
		{  0.f,  1.f,  0.f }, // Down.
		{  0.f,  0.f,  1.f }, // Forward.
		{  0.f,  0.f,  1.f }, // Backwards.
	};

	sr->caster_count = MinValue(R_SceneShadowCasterCount(scene), R_SCENE_MAX_SHADOW_CASTERS);

	R_GPU_ShadowCaster *caster_mapping = GFX_DeviceBufferMap(sr->device, sr->caster_table_buffer);

	for (u32 i = 0; i < sr->caster_count; i++)
	{
		const R_ShadowCaster *info = R_SceneShadowCasterGet(scene, i);
		R_GPU_ShadowCaster *gpu = &caster_mapping[i];

		gpu->position   = info->position;
		gpu->near_plane = info->near;
		gpu->far_plane  = info->far;
		gpu->shadow_map = GFX_DeviceTextureViewBindless(sr->device, sr->shadow_cubemap_views[i]);

		m4 light_proj = M4Perspective(90.f, 1.f, info->near, info->far);

		for (u32 f = 0; f < 6; f++)
		{
			v3 target = V3Add(info->position, light_dirs[f]);
			m4 light_view = M4LookAt(info->position, target, light_ups[f]);
			gpu->face_matrices[f] = M4MulM4(light_proj, light_view);
		}
	}
}

internal void
R_ShadowRendererRender(R_ShadowRenderer *sr,
					   R_Graph *graph,
					   const R_Bulletin *bt,
					   R_Blackboard *bb,
					   const R_Scene *scene,
					   R_Culling *culling)
{
	bb->shadow_data.shadow_caster_table = sr->caster_table_buffer;
	bb->shadow_data.shadow_map_count = sr->caster_count;

	
	// Create one render pass per shadow caster.

	GFX_ShaderKey shader = AST_GetNow(sr->assets, sr->depth_shader, AST_Type_Shader)->shader_data.key;

	for (u32 caster_index = 0; caster_index < sr->caster_count; caster_index++)
	{
		const R_ShadowCaster *info = R_SceneShadowCasterGet(scene, caster_index);

		
		// Build a culling draw stream for this caster's influence sphere.

		R_DrawStream draw_stream = R_CullSphere(culling, graph, bt,
												R_CullFilter_All,
												info->position, info->radius);

		
		// Create the shadow mapping pass.

		// TODO: snprintf the pass name with the caster index for debug labelling.

		R_ShadowMappingPassData *data = ArenaPushArray(bt->pass_arena, R_ShadowMappingPassData, 1);
		data->shader                = shader;
		data->caster_index          = caster_index;
		data->object_buffer_address = bt->scene_resources->object_buffer.gpu;
		data->caster_table_buffer   = sr->caster_table_buffer;
		data->draw_stream           = draw_stream;

		R_GraphTexHandle cubemap_handle = R_GraphImportTexture(graph, sr->shadow_cubemaps[caster_index]);
		bb->shadow_data.shadow_maps[caster_index] = cubemap_handle;

		R_Clear depth_clear = R_ClearDepthStencil(1.f, 0);

		R_Pass *pass = R_GraphAdd(graph, String8Lit("Shadow Mapping"), R_PassType_Graphics);

		R_PassSetRecord        (pass, R_ShadowMappingPassFn, data);
		R_PassSetMultiViewMask (pass, 0b111111);
		R_PassIndirectBuffer   (pass, draw_stream.indirect_buffer);
		R_PassIndirectBuffer   (pass, draw_stream.count_buffer);
		R_PassWriteDepth       (pass, cubemap_handle, &depth_clear);
	}
}
