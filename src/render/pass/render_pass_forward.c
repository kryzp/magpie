
R_PASS_RECORD_DEF(R_ForwardPassFn)
{
	GFX_Device *device            = ctx->device;
	GFX_CmdBuffer *cmd            = ctx->cmd;
	const R_Scene *scene          = ctx->scene;
	const R_ForwardPassData *data = ctx->user_data;

	
	// TODO: replace this system with something more streamlined
	//       i.e: render graph already *knows* we have a depth attachment + # amount of colour formats
	//            on this pass, but there's no utilities to make a pipeline from that info
	//            like imagine something like
	//                R_PassGfxPipelineDef pipeline_def = ...;
	//                GFX_PipelineSt pipeline_st = R_PassFetchPipeline(ctx, pipeline_def)

	GFX_GraphicsPipelineDef pipeline_def = GFX_GraphicsPipelineDefInit(data->shader);
	pipeline_def.has_depth_attachment = true;
	pipeline_def.depth_stencil_state.depth_test_enabled = true;
	pipeline_def.depth_stencil_state.depth_write_enabled = true;
	pipeline_def.colour_attachment_count = 1;
	pipeline_def.colour_attachment_formats[0] = VK_FORMAT_R16G16B16A16_SFLOAT;

	GFX_PipelineSt pipeline_st = GFX_DeviceFetchGraphicsPipeline(device, &pipeline_def);

	GFX_CmdBindBindless(cmd, VK_SHADER_STAGE_ALL_GRAPHICS, pipeline_st.layout);
	GFX_CmdBindPipeline(cmd, pipeline_st.bind_point, pipeline_st.pipeline);
	
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

	args.frame_data_buffer           = GFX_DeviceBufferAddress(device, data->frame_data_buffer);
	args.object_buffer               = data->object_buffer_address;
	args.material_buffer             = R_SceneMaterialBufferAddress(scene);
	args.mesh_buffer                 = R_SceneMeshBufferAddress(scene);

	args.light_buffer                = data->light_buffer_address;
	args.shadow_caster_buffer        = GFX_DeviceBufferAddress(device, data->shadow_caster_table);

	args.irr_sh_buffer               = data->irradiance_sh_buffer_address;
	args.irr_grid_info_buffer        = data->irradiance_grid_info_buffer_address;

	args.irradiance_fallback_cubemap = GFX_DeviceTextureViewBindless(device, R_GraphResolveTextureView(ctx->graph, data->irradiance_fb_handle, GFX_SubresourceRangeAllColour()));
	args.prefilter_cubemap           = GFX_DeviceTextureViewBindless(device, R_GraphResolveTextureView(ctx->graph, data->prefilter_handle,     GFX_SubresourceRangeAllColour()));
	args.brdf_lut                    = GFX_DeviceTextureViewBindless(device, R_GraphResolveTextureView(ctx->graph, data->brdf_handle,          GFX_SubresourceRangeAllColour()));

	args.linear_sampler              = GFX_DeviceSamplerBindless(device, data->linear_sampler);
	args.shadow_sampler              = GFX_DeviceSamplerBindless(device, data->nearest_sampler);

	args.light_count                 = R_SceneGetLightCount(scene);

	GFX_CmdPushConstants(cmd, pipeline_st.layout, VK_SHADER_STAGE_ALL_GRAPHICS, sizeof(args), &args, 0);

	GFX_BufferKey indirect_key = R_GraphResolveBuffer(ctx->graph, data->draw_stream.indirect_buffer);
	GFX_BufferKey counter_key  = R_GraphResolveBuffer(ctx->graph, data->draw_stream.count_buffer);

	R_SceneDrawIndirect(scene, cmd, indirect_key, counter_key);
}

internal void
R_ForwardRendererInit(R_ForwardRenderer *r, GFX_Device *device, AST_Assets *assets)
{
	r->device = device;
	r->assets = assets;
	
	r->shader = AST_Require(assets, String8Lit("assets://shaders/passes/forward/forward.slang"), AST_Type_Shader);
}

internal void
R_ForwardRendererDestroy(R_ForwardRenderer *r)
{
}

internal R_GraphTexHandle
R_ForwardRender(R_ForwardRenderer *r,
				R_Graph *graph,
				R_Blackboard *bb,
				Arena *pass_arena,
				const R_SceneResources *scene_resources,
				GFX_BufferKey frame_data_buffer,
				GFX_SamplerKey linear_sampler,
				GFX_SamplerKey nearest_sampler,
				const R_IrradianceVolume *irradiance_volume,
				GFX_TextureKey irradiance_fallback,
				GFX_TextureKey prefilter,
				GFX_TextureKey brdf,
				const R_DrawStream *draw_stream)
{
	const R_BB_ShadowData *shadow = &bb->shadow_data;

	R_Clear depth_clear  = R_ClearDepthStencil(1.f, 0);
	R_Clear colour_clear = R_ClearColour(0.f, 0.f, 0.f, 1.f);

	R_TextureInfo depth_info = R_TextureInfoInitDepth(graph->device);

	R_TextureInfo lighting_info = R_TextureInfoInit();
	lighting_info.format = VK_FORMAT_R16G16B16A16_SFLOAT;
	lighting_info.flags  = GFX_TextureAllocFlag_Storage;

	R_GraphTexHandle lighting = R_GraphCreateTexture(graph, &lighting_info);
	R_GraphTexHandle depth    = R_GraphCreateTexture(graph, &depth_info);

	R_Pass *pass = R_GraphAdd(graph, String8Lit("Forward"), R_PassType_Graphics);

	lighting = R_PassWriteColour (pass, lighting, &colour_clear);
	depth    = R_PassWriteDepth  (pass, depth,    &depth_clear);

	bb->depth = depth;

	R_PassIndirectBuffer(pass, draw_stream->indirect_buffer);
	R_PassIndirectBuffer(pass, draw_stream->count_buffer);

	R_GraphTexHandle irradiance_fb_handle = R_PassReadTextureGraphics(pass, R_GraphImportTexture(graph, irradiance_fallback));
	R_GraphTexHandle prefilter_handle     = R_PassReadTextureGraphics(pass, R_GraphImportTexture(graph, prefilter));
	R_GraphTexHandle brdf_handle          = R_PassReadTextureGraphics(pass, R_GraphImportTexture(graph, brdf));

	for (u32 i = 0; i < shadow->shadow_map_count; i++)
		R_PassReadTextureGraphics(pass, shadow->shadow_maps[i]);

	GFX_ShaderKey shader = AST_GetNow(r->assets, r->shader, AST_Type_Shader)->shader.key;

	R_ForwardPassData *data = ArenaPushArray(pass_arena, R_ForwardPassData, 1);
	data->shader                = shader;
	data->frame_data_buffer     = frame_data_buffer;
	data->shadow_caster_table   = shadow->shadow_caster_table;
	data->linear_sampler        = linear_sampler;
	data->nearest_sampler       = nearest_sampler;
	data->object_buffer_address = scene_resources->object_buffer.gpu;
	data->light_buffer_address  = scene_resources->light_buffer.gpu;
	data->irradiance_fb_handle  = irradiance_fb_handle;
	data->prefilter_handle      = prefilter_handle;
	data->brdf_handle           = brdf_handle;
	data->draw_stream           = *draw_stream;

	if (R_IrradianceVolumeIsBaked(irradiance_volume))
	{
		data->irradiance_sh_buffer_address        = GFX_DeviceBufferAddress(graph->device, R_IrradianceVolumeGetSHBuffer(irradiance_volume));
		data->irradiance_grid_info_buffer_address = GFX_DeviceBufferAddress(graph->device, R_IrradianceVolumeGetGridInfoBuffer(irradiance_volume));
	}
	else
	{
		data->irradiance_sh_buffer_address        = 0;
		data->irradiance_grid_info_buffer_address = 0;
	}

	R_PassSetRecord(pass, R_ForwardPassFn, data);

	return lighting;
}
