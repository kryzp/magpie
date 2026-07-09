
static R_PASS_RECORD_DEF(R_ForwardPassFn)
{
	G_CmdBuffer *cmd = ctx->cmd;
	const R_ForwardPassData *data = ctx->user_data;
	const R_FrameParams *frame_params = data->frame_params;

	G_GraphicsPipelineDef pipeline_def = G_GraphicsPipelineDefFromInfo(data->shader, ctx->render_info);
	pipeline_def.depth_stencil_state.depth_test_enabled = true;
	pipeline_def.depth_stencil_state.depth_write_enabled = true;

	G_PipelineSt pipeline_st = G_DeviceFetchGraphicsPipeline(&pipeline_def);

	G_CmdBindBindless(cmd, VK_SHADER_STAGE_ALL_GRAPHICS, pipeline_st.layout);
	G_CmdBindPipeline(cmd, pipeline_st.bind_point, pipeline_st.pipeline);

	struct
	{
		u64 frame_data_buffer;
		u64 object_buffer;
		u64 material_buffer;
		u64 mesh_buffer;

		u64 light_buffer;
		u64 shadow_caster_buffer;

		u64 irr_sh_buffer;
		u64 irr_grid_info_buffer;

		u32 irradiance_fallback_cubemap;
		u32 prefilter_cubemap;
		u32 brdf_lut;

		u32 linear_sampler;
		u32 shadow_sampler;

		u32 light_count;
	}
	args;

	args.frame_data_buffer = frame_params->frame_data.gpu;
	args.object_buffer = frame_params->object_buffer.gpu;
	args.material_buffer = G_DeviceBufferAddress(frame_params->material_buffer);
	args.mesh_buffer = G_DeviceBufferAddress(frame_params->mesh_buffer);

	args.light_buffer = frame_params->light_buffer.gpu;
	args.shadow_caster_buffer = G_DeviceBufferAddress(data->shadow_caster_table);

	args.irr_sh_buffer = data->irradiance_sh_buffer_address;
	args.irr_grid_info_buffer = data->irradiance_grid_info_buffer_address;

	args.irradiance_fallback_cubemap = G_DeviceTextureViewBindless(R_GraphResolveTextureView(ctx->graph, data->irradiance_fb_handle, G_SubresourceRangeAllColour()));
	args.prefilter_cubemap = G_DeviceTextureViewBindless(R_GraphResolveTextureView(ctx->graph, data->prefilter_handle, G_SubresourceRangeAllColour()));
	args.brdf_lut = G_DeviceTextureViewBindless(R_GraphResolveTextureView(ctx->graph, data->brdf_handle, G_SubresourceRangeAllColour()));

	args.linear_sampler = G_DeviceSamplerBindless(frame_params->linear_sampler);
	args.shadow_sampler = G_DeviceSamplerBindless(frame_params->nearest_sampler);

	args.light_count = frame_params->light_count;

	G_CmdPushConstants(cmd, pipeline_st.layout, VK_SHADER_STAGE_ALL_GRAPHICS, args, 0);

	G_BufferKey indirect_key = R_GraphResolveBuffer(ctx->graph, data->draw_stream.indirect_buffer);
	G_BufferKey counter_key = R_GraphResolveBuffer(ctx->graph, data->draw_stream.count_buffer);

	R_FrameParamsDrawIndirect(frame_params, cmd, indirect_key, counter_key);
}

static void R_ForwardRender(R_Graph *graph,
							const R_FrameParams *frame_params,
							R_Blackboard *bb,
							const R_DrawStream *draw_stream)
{
	R_Pass *pass = R_GraphAdd(graph, String8Lit("Forward"), R_PassType_Graphics);

	bb->lighting.msaa = R_PassWriteColour(pass, bb->lighting.msaa, NULL);
	bb->depth.msaa = R_PassWriteDepth(pass, bb->depth.msaa, NULL);

	R_PassIndirectBuffer(pass, draw_stream->indirect_buffer);
	R_PassIndirectBuffer(pass, draw_stream->count_buffer);

	R_GraphTexHandle irradiance_fb_handle = R_PassReadTextureGraphics(pass, R_GraphImportTexture(graph, frame_params->irradiance_fallback_cubemap));
	R_GraphTexHandle prefilter_handle = R_PassReadTextureGraphics(pass, R_GraphImportTexture(graph, frame_params->prefilter_cubemap));
	R_GraphTexHandle brdf_handle = R_PassReadTextureGraphics(pass, R_GraphImportTexture(graph, frame_params->brdf));

	for (u32 i = 0; i < bb->shadow_map_count; i++)
		R_PassReadTextureGraphics(pass, bb->shadow_maps[i]);

	A_Handle shader_handle = A_Require(String8Lit("assets://shaders/passes/forward/forward.slang"), A_Type_Shader);
	G_ShaderKey shader = A_GetNow(shader_handle)->shader.key;

	R_ForwardPassData *data = ArenaPushArray(frame_params->arena, R_ForwardPassData, 1);

	data->shader = shader;
	data->frame_params = frame_params;
	data->shadow_caster_table = bb->shadow_caster_table;
	data->irradiance_fb_handle = irradiance_fb_handle;
	data->prefilter_handle = prefilter_handle;
	data->brdf_handle = brdf_handle;
	data->draw_stream = *draw_stream;

	if (/*R_IrradianceVolumeIsBaked(irradiance_volume)*/ false)
	{
		//data->irradiance_sh_buffer_address        = G_DeviceBufferAddress(R_IrradianceVolumeGetSHBuffer(irradiance_volume));
		//data->irradiance_grid_info_buffer_address = G_DeviceBufferAddress(R_IrradianceVolumeGetGridInfoBuffer(irradiance_volume));
	}
	else
	{
		data->irradiance_sh_buffer_address = 0;
		data->irradiance_grid_info_buffer_address = 0;
	}

	R_PassSetRecord(pass, R_ForwardPassFn, data);
}
