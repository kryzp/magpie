
R_PASS_RECORD_DEF(R_ForwardPassFn)
{
	G_Device *device            = ctx->device;
	G_CmdBuffer *cmd            = ctx->cmd;
	const R_Scene *scene          = ctx->scene;
	const R_ForwardPassData *data = ctx->user_data;

	G_GraphicsPipelineDef pipeline_def = G_GraphicsPipelineDefFromInfo(data->shader, ctx->render_info);
	pipeline_def.depth_stencil_state.depth_test_enabled = true;
	pipeline_def.depth_stencil_state.depth_write_enabled = true;

	G_PipelineSt pipeline_st = G_DeviceFetchGraphicsPipeline(device, &pipeline_def);

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

	args.frame_data_buffer           = G_DeviceBufferAddress(device, data->frame_data_buffer);
	args.object_buffer               = data->object_buffer_address;
	args.material_buffer             = R_SceneMaterialBufferAddr(scene);
	args.mesh_buffer                 = R_SceneMeshBufferAddr(scene);

	args.light_buffer                = data->light_buffer_address;
	args.shadow_caster_buffer        = G_DeviceBufferAddress(device, data->shadow_caster_table);

	args.irr_sh_buffer               = data->irradiance_sh_buffer_address;
	args.irr_grid_info_buffer        = data->irradiance_grid_info_buffer_address;

	args.irradiance_fallback_cubemap = G_DeviceTextureViewBindless(device, R_GraphResolveTextureView(ctx->graph, data->irradiance_fb_handle, G_SubresourceRangeAllColour()));
	args.prefilter_cubemap           = G_DeviceTextureViewBindless(device, R_GraphResolveTextureView(ctx->graph, data->prefilter_handle,     G_SubresourceRangeAllColour()));
	args.brdf_lut                    = G_DeviceTextureViewBindless(device, R_GraphResolveTextureView(ctx->graph, data->brdf_handle,          G_SubresourceRangeAllColour()));

	args.linear_sampler              = G_DeviceSamplerBindless(device, data->linear_sampler);
	args.shadow_sampler              = G_DeviceSamplerBindless(device, data->nearest_sampler);

	args.light_count                 = R_SceneLightCount(scene);

	G_CmdPushConstants(cmd, pipeline_st.layout, VK_SHADER_STAGE_ALL_GRAPHICS, sizeof(args), &args, 0);
	
	G_BufferKey indirect_key = R_GraphResolveBuffer(ctx->graph, data->draw_stream.indirect_buffer);
	G_BufferKey counter_key  = R_GraphResolveBuffer(ctx->graph, data->draw_stream.count_buffer);

	R_SceneDrawIndirect(scene, cmd, indirect_key, counter_key);
}

internal void
R_ForwardRendererInit(R_ForwardRenderer *r, G_Device *device, A_Registry *assets)
{
	r->device = device;
	r->assets = assets;
	
	r->shader = A_Require(assets, String8Lit("assets://shaders/passes/forward/forward.slang"), A_Type_Shader);
}

internal void
R_ForwardRendererDestroy(R_ForwardRenderer *r)
{
}

internal void
R_ForwardRender(R_ForwardRenderer *r,
				R_Graph *graph,
				const R_Bulletin *bt,
				R_Blackboard *bb,
				const R_DrawStream *draw_stream)
{
	const R_BB_ShadowData *bb_shadow = &bb->shadow_data;

	R_Clear colour_clear = R_ClearColour(0.f, 0.f, 0.f, 1.f);
	R_Clear depth_clear  = R_ClearDepthStencil(1.f, 0);

	R_TextureInfo depth_info = R_TextureInfoInitSwapchain(graph->device->context.depth_format, v3(1.f, 1.f, 1.f));

	R_TextureInfo lighting_info = R_TextureInfoInitSwapchain(VK_FORMAT_R16G16B16A16_SFLOAT, v3(1.f, 1.f, 1.f));
	lighting_info.flags = G_TextureAllocFlag_Storage;

	bb->lighting = R_GraphCreateMsaa(graph, &lighting_info, VK_SAMPLE_COUNT_4_BIT);
	bb->depth    = R_GraphCreateMsaa(graph, &depth_info,    VK_SAMPLE_COUNT_4_BIT);

	R_Pass *pass = R_GraphAdd(graph, String8Lit("Forward"), R_PassType_Graphics);

	bb->lighting.msaa = R_PassWriteColour (pass, bb->lighting.msaa, &colour_clear);
	bb->depth.msaa    = R_PassWriteDepth  (pass, bb->depth.msaa,    &depth_clear);

	R_PassIndirectBuffer(pass, draw_stream->indirect_buffer);
	R_PassIndirectBuffer(pass, draw_stream->count_buffer);

	R_GraphTexHandle irradiance_fb_handle = R_PassReadTextureGraphics(pass, R_GraphImportTexture(graph, bt->irradiance_fallback_cubemap));
	R_GraphTexHandle prefilter_handle     = R_PassReadTextureGraphics(pass, R_GraphImportTexture(graph, bt->prefilter_cubemap));
	R_GraphTexHandle brdf_handle          = R_PassReadTextureGraphics(pass, R_GraphImportTexture(graph, bt->brdf));

	for (u32 i = 0; i < bb_shadow->shadow_map_count; i++)
		R_PassReadTextureGraphics(pass, bb_shadow->shadow_maps[i]);

	G_ShaderKey shader = A_GetNow(r->assets, r->shader, A_Type_Shader)->shader.key;

	R_ForwardPassData *data = ArenaPushArray(bt->pass_arena, R_ForwardPassData, 1);

	data->shader                          = shader;

	data->frame_data_buffer               = bt->frame_data_buffer;
	data->shadow_caster_table             = bb_shadow->shadow_caster_table;

	data->linear_sampler                  = bt->linear_sampler;
	data->nearest_sampler                 = bt->nearest_sampler;

	data->object_buffer_address           = bt->scene_resources->object_buffer.gpu;
	data->light_buffer_address            = bt->scene_resources->light_buffer.gpu;

	data->irradiance_fb_handle            = irradiance_fb_handle;
	data->prefilter_handle                = prefilter_handle;
	data->brdf_handle                     = brdf_handle;

	data->draw_stream                     = *draw_stream;

	if (R_IrradianceVolumeIsBaked(bt->irradiance_volume))
	{
		data->irradiance_sh_buffer_address        = G_DeviceBufferAddress(graph->device, R_IrradianceVolumeGetSHBuffer       (bt->irradiance_volume));
		data->irradiance_grid_info_buffer_address = G_DeviceBufferAddress(graph->device, R_IrradianceVolumeGetGridInfoBuffer (bt->irradiance_volume));
	}
	else
	{
		data->irradiance_sh_buffer_address        = 0;
		data->irradiance_grid_info_buffer_address = 0;
	}

	R_PassSetRecord(pass, R_ForwardPassFn, data);
}
