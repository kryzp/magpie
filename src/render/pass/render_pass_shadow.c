
internal R_PASS_RECORD_DEF(R_ShadowMappingPassFn)
{
	G_CmdBuffer *cmd = ctx->cmd;
	const R_ShadowMappingPassData *data = ctx->user_data;
	const R_FrameParams *frame_params = data->frame_params;
	
	G_GraphicsPipelineDef pipeline_def = G_GraphicsPipelineDefFromInfo(frame_params->shadow_shader, ctx->render_info);
	pipeline_def.depth_stencil_state.depth_test_enabled = true;
	pipeline_def.depth_stencil_state.depth_write_enabled = true;

	G_PipelineSt pipeline_st = G_DeviceFetchGraphicsPipeline(&pipeline_def);

	struct
	{
		u64 object_buffer;
		u64 mesh_buffer;
		u64 caster_data_buffer;
		u32 caster_index;
	}
	pc;

	pc.object_buffer = frame_params->object_buffer.gpu;
	pc.mesh_buffer = G_DeviceBufferAddress(frame_params->mesh_buffer);
	pc.caster_data_buffer = G_DeviceBufferAddress(data->caster_table_buffer);
	pc.caster_index = data->caster_index;

	G_CmdBindBindless(cmd, VK_SHADER_STAGE_ALL_GRAPHICS, pipeline_st.layout);
	G_CmdBindPipeline(cmd, pipeline_st.bind_point, pipeline_st.pipeline);
	G_CmdPushConstants(cmd, pipeline_st.layout, VK_SHADER_STAGE_ALL_GRAPHICS, pc, 0);

	G_ResourceKey indirect_key = R_GraphResolveBuffer(ctx->graph, data->draw_stream.indirect_buffer);
	G_ResourceKey counter_key = R_GraphResolveBuffer(ctx->graph, data->draw_stream.count_buffer);

	R_FrameParamsDrawIndirect(frame_params, cmd, indirect_key, counter_key);
}

internal void R_ShadowsInit(R_ShadowState *st)
{
	G_BufferAllocInfo caster_buf_info = {0};
	caster_buf_info.usage = VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT;
	caster_buf_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	caster_buf_info.size = R_FRAME_PARAMS_MAX_SHADOW_CASTERS * sizeof(R_GPU_ShadowCaster);

	st->caster_table_buffer = G_DeviceBufferAlloc(&caster_buf_info);

	for (u32 i = 0; i < R_FRAME_PARAMS_MAX_SHADOW_CASTERS; i++)
	{
		st->shadow_cubemaps[i] = G_DeviceTextureAllocCubemapDepth(R_SHADOW_MAP_RESOLUTION, 1);

		G_TextureViewCreateInfo view_info = {0};
		view_info.texture = st->shadow_cubemaps[i];
		view_info.type = VK_IMAGE_VIEW_TYPE_CUBE;
		view_info.range = G_SubresourceRangeAllDepth();

		st->shadow_cubemap_views[i] = G_DeviceTextureViewFetch(&view_info);
	}
}

internal void R_ShadowsDestroy(R_ShadowState *st)
{
	G_DeviceBufferDestroy(st->caster_table_buffer);

	for (u32 i = 0; i < R_FRAME_PARAMS_MAX_SHADOW_CASTERS; i++)
		G_DeviceTextureDestroy(st->shadow_cubemaps[i]);
}

internal void R_ShadowsUploadGPU(R_ShadowState *st, const R_FrameParams *frame_params)
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

	st->caster_count = MinValue(frame_params->shadow_caster_count, R_FRAME_PARAMS_MAX_SHADOW_CASTERS);

	R_GPU_ShadowCaster *caster_mapping = G_DeviceBufferMap(st->caster_table_buffer);

	for (u32 i = 0; i < st->caster_count; i++)
	{
		const R_ShadowCaster *info = &frame_params->shadow_casters[i];
		R_GPU_ShadowCaster *gpu = &caster_mapping[i];

		gpu->position = info->position;
		gpu->near_plane = info->near;
		gpu->far_plane = info->far;
		gpu->shadow_map = G_DeviceTextureViewBindless(st->shadow_cubemap_views[i]);

		m4 light_proj = M4Perspective(90.f, 1.f, info->near, info->far);

		for (u32 f = 0; f < 6; f++)
		{
			v3 target = V3Add(info->position, light_dirs[f]);
			m4 light_view = M4LookAt(info->position, target, light_ups[f]);
			gpu->face_matrices[f] = M4MulM4(light_proj, light_view);
		}
	}
}

internal void R_ShadowsRender(R_ShadowState *st,
							R_Graph *graph,
							const R_FrameParams *frame_params,
							R_Blackboard *bb)
{
	bb->shadow_caster_table = st->caster_table_buffer;
	bb->shadow_map_count = st->caster_count;

	for (u32 caster_index = 0; caster_index < st->caster_count; caster_index++)
	{
		const R_ShadowCaster *info = &frame_params->shadow_casters[caster_index];
		
		R_DrawStream draw_stream = R_CullSphere(graph, frame_params,
												R_CullFilter_All,
												info->position, info->radius);

		// TODO: snprintf the pass name with the caster index for debug labelling.
		R_ShadowMappingPassData *data = ArenaPushArray(frame_params->arena, R_ShadowMappingPassData, 1);
		data->frame_params = frame_params;
		data->caster_index = caster_index;
		data->caster_table_buffer = st->caster_table_buffer;
		data->draw_stream = draw_stream;

		R_GraphTexHandle cubemap_handle = R_GraphImportTexture(graph, st->shadow_cubemaps[caster_index]);
		bb->shadow_maps[caster_index] = cubemap_handle;

		R_Clear depth_clear = R_ClearDepthStencil(1.f, 0);

		R_Pass *pass = R_GraphAdd(graph, String8Lit("Shadow Mapping"), R_PassType_Graphics);

		R_PassSetRecord(pass, R_ShadowMappingPassFn, data);
		R_PassSetMultiViewMask(pass, 0b111111);
		R_PassIndirectBuffer(pass, draw_stream.indirect_buffer);
		R_PassIndirectBuffer(pass, draw_stream.count_buffer);
		R_PassWriteDepth(pass, cubemap_handle, &depth_clear);
	}
}
