
internal void
R_SystemCreateSkyboxMesh(R_System *s)
{
	static v3 vertices[] = {
		{ -1.f,  1.f,  1.f },
		{ -1.f,  1.f, -1.f },
		{  1.f,  1.f, -1.f },
		{  1.f,  1.f,  1.f },
		{ -1.f, -1.f,  1.f },
		{ -1.f, -1.f, -1.f },
		{  1.f, -1.f, -1.f },
		{  1.f, -1.f,  1.f }
	};

	static u16 indices[] = {
		1, 2, 0,
		3, 0, 2,

		6, 5, 7,
		4, 7, 5,

		5, 1, 4,
		0, 4, 1,

		2, 6, 3,
		7, 3, 6,

		5, 6, 1,
		2, 1, 6,

		0, 3, 4,
		7, 4, 3
	};

	R_MeshAlloc(&s->skybox_mesh, s->device,
				sizeof(v3), VK_INDEX_TYPE_UINT16,
				ArraySize(vertices), ArraySize(indices));

	G_BufferKey staging_buffer = G_DeviceStageAlloc(s->device, R_MeshVertexBufferSize(&s->skybox_mesh) + R_MeshIndexBufferSize(&s->skybox_mesh));

	R_MeshWriteToStage(&s->skybox_mesh, s->device,
					   staging_buffer, 0,
					   vertices, indices);

	{
		G_CmdBuffer cmd = G_DeviceSubmitImBegin(s->device);
		R_MeshUpload(&s->skybox_mesh, &cmd, staging_buffer, 0);
		G_DeviceSubmitImEnd(s->device, &cmd);
	}

	G_DeviceBufferDestroy(s->device, staging_buffer);
}

internal void
R_SystemInit(R_System *s, Arena *arena, G_Device *device, A_Registry *assets, LOG_Channel log_channel)
{
	s->arena = arena;
	s->device = device;
	s->assets = assets;
	s->log_channel = log_channel;
	
	G_BufferAllocInfo frame_buffer_alloc_info = {0};
	frame_buffer_alloc_info.usage = VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT;
	frame_buffer_alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	frame_buffer_alloc_info.size = sizeof(R_GPU_FrameData);

	s->frame_data_buffer = G_DeviceBufferAlloc(s->device, &frame_buffer_alloc_info);
	
	m4 capture_view_matrices[] = {
		M4LookAt(v3(0.f, 0.f, 0.f), v3( 1.f, 0.f, 0.f), v3( 0.f, 0.f, 1.f)), // Right.
		M4LookAt(v3(0.f, 0.f, 0.f), v3(-1.f, 0.f, 0.f), v3( 0.f, 0.f, 1.f)), // Left.
		M4LookAt(v3(0.f, 0.f, 0.f), v3( 0.f, 0.f, 1.f), v3( 0.f,-1.f, 0.f)), // Up.
		M4LookAt(v3(0.f, 0.f, 0.f), v3( 0.f, 0.f,-1.f), v3( 0.f, 1.f, 0.f)), // Down.
		M4LookAt(v3(0.f, 0.f, 0.f), v3( 0.f, 1.f, 0.f), v3( 0.f, 0.f, 1.f)), // Forward.
		M4LookAt(v3(0.f, 0.f, 0.f), v3( 0.f,-1.f, 0.f), v3( 0.f, 0.f, 1.f)), // Backwards.
	};

	m4 capture_projection_matrix = M4Perspective(90.f, 1.f, 0.1f, 10.f);

	for (u32 i = 0; i < 6; i++)
		capture_view_matrices[i] = M4MulM4(capture_projection_matrix, capture_view_matrices[i]);

	G_BufferAllocInfo cubemap_capture_buffer_alloc_info = {0};
	cubemap_capture_buffer_alloc_info.usage = VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_2_TRANSFER_DST_BIT;
	cubemap_capture_buffer_alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	cubemap_capture_buffer_alloc_info.size = sizeof(capture_view_matrices);

	s->cubemap_capture_transform_buffer = G_DeviceBufferAlloc(s->device, &cubemap_capture_buffer_alloc_info);
	
	G_DeviceBufferWrite(s->device,
						s->cubemap_capture_transform_buffer,
						capture_view_matrices,
						sizeof(capture_view_matrices), 0);
	
	s->linear_sampler  = G_DeviceSamplerCreateF(s->device, VK_FILTER_LINEAR);
	s->nearest_sampler = G_DeviceSamplerCreateF(s->device, VK_FILTER_NEAREST);
	
	R_SystemCreateSkyboxMesh(s);

	R_CullingInit                (&s->culling,                               s->assets);
	R_ShadowRendererInit         (&s->shadow_renderer,            s->device, s->assets);
	R_ForwardRendererInit        (&s->forward_renderer,           s->device, s->assets);
	R_DebugRendererInitAndSelect (&s->debug_renderer,   s->arena, s->device, s->assets);
	
	DebugLogI(s->log_channel, "Initialized.");
}

internal void
R_SystemDestroy(R_System *s)
{
	R_IrradianceVolumeDestroy (&s->irradiance_volume);
	R_DebugRendererDestroy    (&s->debug_renderer);
	R_ForwardRendererDestroy  (&s->forward_renderer);
	R_ShadowRendererDestroy   (&s->shadow_renderer);
	R_CullingDestroy          (&s->culling);

	G_DeviceTextureDestroy(s->device, s->brdf_lut);
	G_DeviceTextureDestroy(s->device, s->environment_cubemap);
	G_DeviceTextureDestroy(s->device, s->irradiance_cubemap);
	G_DeviceTextureDestroy(s->device, s->prefilter_cubemap);
	
	R_MeshDestroy(&s->skybox_mesh, s->device);
	
	G_DeviceSamplerDestroy(s->device, s->linear_sampler);
	G_DeviceSamplerDestroy(s->device, s->nearest_sampler);

	G_DeviceBufferDestroy(s->device, s->frame_data_buffer);
	G_DeviceBufferDestroy(s->device, s->cubemap_capture_transform_buffer);

	DebugLogI(s->log_channel, "Destroyed.");
}

internal void
R_SystemGenerateLookupsAndMaps(R_System *s, R_Graph *g, Arena *arena)
{
	const u32 prefilter_mips = 5;

	s->brdf_lut            = G_DeviceTextureAlloc2D      (s->device, 512, 512, VK_FORMAT_R32G32_SFLOAT,       1);
	s->environment_cubemap = G_DeviceTextureAllocCubemap (s->device, 512,      VK_FORMAT_R32G32B32A32_SFLOAT, 8);
	s->irradiance_cubemap  = G_DeviceTextureAllocCubemap (s->device,  32,      VK_FORMAT_R32G32B32A32_SFLOAT, 1);
	s->prefilter_cubemap   = G_DeviceTextureAllocCubemap (s->device, 128,      VK_FORMAT_R32G32B32A32_SFLOAT, prefilter_mips);
	
	A_Handle brdf_lut_shader_handle    = A_Require(s->assets, String8Lit("assets://shaders/passes/ibl/brdf_lut.slang"),                   A_Type_Shader);
	A_Handle hdr_to_env_shader_handle  = A_Require(s->assets, String8Lit("assets://shaders/passes/ibl/hdr_to_environment_cubemap.slang"), A_Type_Shader);
	A_Handle irradiance_shader_handle  = A_Require(s->assets, String8Lit("assets://shaders/passes/ibl/irradiance_convolution.slang"),     A_Type_Shader);
	A_Handle prefilter_shader_handle   = A_Require(s->assets, String8Lit("assets://shaders/passes/ibl/prefilter_convolution.slang"),      A_Type_Shader);
	A_Handle hdr_texture_handle        = A_Require(s->assets, String8Lit("assets://environment_map_1.hdr"),                               A_Type_Texture);
	
	G_ShaderKey brdf_lut_shader        = A_GetNow(s->assets, brdf_lut_shader_handle)->shader.key;
	G_ShaderKey hdr_to_env_shader      = A_GetNow(s->assets, hdr_to_env_shader_handle)->shader.key;
	G_ShaderKey irradiance_pass_shader = A_GetNow(s->assets, irradiance_shader_handle)->shader.key;
	G_ShaderKey prefilter_pass_shader  = A_GetNow(s->assets, prefilter_shader_handle)->shader.key;
	G_TextureKey hdr_texture_gfx       = A_GetNow(s->assets, hdr_texture_handle)->texture.key;

	// Generate BRDF Lookup Table.
	{
		R_BRDFLutPassData *data = ArenaPushArray(arena, R_BRDFLutPassData, 1);
		data->shader = brdf_lut_shader;
		
		R_Pass *pass = R_GraphAdd(g, String8Lit("BRDF LUT"), R_PassType_Graphics);
		R_PassSetRecord(pass, R_BRDFLutPassFn, data);
		R_PassWriteColour(pass, R_GraphImportTexture(g, s->brdf_lut), NULL);
	}
	
	// Generate Environment Cubemap.
	{
		R_HdrToEnvPassData *data = ArenaPushArray(arena, R_HdrToEnvPassData, 1);
		data->shader = hdr_to_env_shader;
		data->sampler = s->linear_sampler;
		data->hdr_view = G_DeviceTextureViewAuto(s->device, hdr_texture_gfx);
		data->capture_transforms = s->cubemap_capture_transform_buffer;
		data->skybox_mesh = &s->skybox_mesh;
		
		R_Pass *pass = R_GraphAdd(g, String8Lit("HDR -> Environment Map"), R_PassType_Graphics);
		R_PassSetRecord(pass, R_HdrToEnvPassFn, data);
		R_PassSetMultiViewMask(pass, 0b111111);
		R_PassWriteColour(pass, R_GraphImportTexture(g, s->environment_cubemap), NULL);

		R_GenerateMipsPassData *mips_data = ArenaPushArray(arena, R_GenerateMipsPassData, 1);
		mips_data->texture = s->environment_cubemap;
		
		R_Pass *pass_mipmaps = R_GraphAdd(g, String8Lit("Environment Map Mipmapping"), R_PassType_Transfer);
		R_PassSetRecord(pass_mipmaps, R_GenerateMipsPassFn, mips_data);
		R_PassBlitTextureDst(pass_mipmaps, R_GraphImportTexture(g, s->environment_cubemap));
	}
	
	// Irradiance.
	{
		R_IBLPassIrradianceData *data = ArenaPushArray(arena, R_IBLPassIrradianceData, 1);
		data->shader = irradiance_pass_shader;
		data->sampler = s->linear_sampler;
		data->env_view = G_DeviceTextureViewAuto(s->device, s->environment_cubemap);
		data->capture_transforms = s->cubemap_capture_transform_buffer;
		data->skybox_mesh = &s->skybox_mesh;
		
		R_Pass *pass = R_GraphAdd(g, String8Lit("Irradiance"), R_PassType_Graphics);
		R_PassSetRecord(pass, R_IBLPassIrradianceFn, data);
		R_PassSetMultiViewMask(pass, 0b111111);
		R_PassWriteColour(pass, R_GraphImportTexture(g, s->irradiance_cubemap), NULL);
	}
	
	// Prefilter.
	{
		const u32 mipmap_count = prefilter_mips;
		
		for (u32 i = 0; i < mipmap_count; i++)
		{
			R_IBLPassPrefilterData *data = ArenaPushArray(arena, R_IBLPassPrefilterData, 1);
			data->shader = prefilter_pass_shader;
			data->sampler = s->linear_sampler;
			data->env_view = G_DeviceTextureViewAuto(s->device, s->environment_cubemap);
			data->capture_transforms = s->cubemap_capture_transform_buffer;
			data->skybox_mesh = &s->skybox_mesh;
			data->roughness = (f32)i / (f32)(mipmap_count - 1);

			G_SubresourceRange range = {0};
			range.aspects = VK_IMAGE_ASPECT_COLOR_BIT;
			range.base_mip = i;
			range.mips = 1;
			range.base_layer = 0;
			range.layers = 6;
		
			R_Pass *pass = R_GraphAdd(g, String8Lit("Prefilter"), R_PassType_Graphics);
			R_PassSetRecord(pass, R_IBLPassPrefilterFn, data);
			R_PassSetMultiViewMask(pass, 0b111111);
			R_PassWriteColourEx(pass, R_GraphImportTexture(g, s->prefilter_cubemap), NULL, range);
		}
	}
	
	R_IrradianceVolumeInit(&s->irradiance_volume,
						   s->device, s->assets,
						   osapi->LogChannelOpenFrom(s->log_channel, String8Lit("IRRADIANCE")),
						   v3(-8.f, -6.f,  -1.f),
						   v3( 8.f,  6.f,  12.f),
						   1, 1, 1,
						   &s->skybox_mesh,
						   G_DeviceTextureViewAuto(s->device, s->environment_cubemap),
						   s->linear_sampler);
	
	DebugLogI(s->log_channel, "Generated lookups and maps.");
}

internal void
R_SystemRender(R_System *s, R_Graph *g, const R_FrameParams *f)
{
	R_FrustumVolume frustum = R_CameraFrustum(&f->camera);
	
	u32 window_width, window_height;
	osapi->GetWindowSize(&window_width, &window_height);
	
	R_GPU_FrameData frame_data = {0};
	frame_data.view = f->camera.view;
	frame_data.proj = f->camera.proj;
	frame_data.view_proj = M4MulM4(f->camera.proj, f->camera.view);
	frame_data.view_proj_no_translation = M4MulM4(f->camera.proj, M4RemoveTranslation(f->camera.view));
	frame_data.inv_view = M4Inverse(f->camera.view);
	frame_data.inv_proj = M4Inverse(f->camera.proj);
	frame_data.camera_position = f->camera.position;
	frame_data.window_resolution = v2(window_width, window_height);
	frame_data.time = f->elapsed;

	G_DeviceBufferWrite(s->device,
						s->frame_data_buffer,
						&frame_data, sizeof(frame_data), 0);

	R_Bulletin bt = {0};
	bt.pass_arena = f->arena;
	bt.frame_data_buffer = s->frame_data_buffer;
	bt.brdf = s->brdf_lut;
	bt.linear_sampler = s->linear_sampler;
	bt.nearest_sampler = s->nearest_sampler;
	bt.scene_resources = &f->scene_data;
	bt.irradiance_volume = &s->irradiance_volume;
	bt.irradiance_fallback_cubemap = s->irradiance_cubemap;
	bt.prefilter_cubemap = s->prefilter_cubemap;
	bt.brdf = s->brdf_lut;
	
	R_Blackboard bb = {0};

	R_ShadowRendererUploadGPU(&s->shadow_renderer, &bt);

	R_ShadowRendererRender(&s->shadow_renderer, g, &bt, &bb, &s->culling);

	R_DrawStream draw_stream = R_CullFrustum(&s->culling, g, &bt, R_CullFilter_OpaqueOnly, &frustum);

	R_ForwardRender(&s->forward_renderer, g, &bt, &bb, &draw_stream);
		
	// Skybox.
	{
		A_Handle shader_handle = A_Require(s->assets, String8Lit("assets://shaders/passes/post/skybox.slang"), A_Type_Shader);
		G_ShaderKey shader = A_GetNow(s->assets, shader_handle)->shader.key;

		R_SkyboxPassData *data = ArenaPushArray(f->arena, R_SkyboxPassData, 1);
		data->shader = shader;
		data->cubemap = G_DeviceTextureViewAuto(s->device, s->environment_cubemap);
		data->sampler = s->linear_sampler;
		data->frame_data_buffer = s->frame_data_buffer;
		data->skybox_mesh = &s->skybox_mesh;

		R_Pass *pass = R_GraphAdd(g, String8Lit("Skybox"), R_PassType_Graphics);
		bb.lighting.resolved = R_PassWriteColourResolve(pass, bb.lighting.msaa, bb.lighting.resolved, NULL);
		bb.depth.resolved = R_PassWriteDepthResolve(pass, bb.depth.msaa,    bb.depth.resolved,    NULL);
		R_PassReadTextureGraphics(pass, R_GraphImportTexture(g, s->environment_cubemap));
		R_PassSetRecord(pass, R_SkyboxPassFn, data);
	}
	
	// Post Processing.
	{
		A_Handle shader_handle = A_Require(s->assets, String8Lit("assets://shaders/passes/post/hdr_tonemapping.slang"), A_Type_Shader);
		G_ShaderKey shader = A_GetNow(s->assets, shader_handle)->shader.key;

		R_PostProcessingPassData *data = ArenaPushArray(f->arena, R_PostProcessingPassData, 1);
		data->shader = shader;
		data->exposure = 1.f;
		data->input = bb.lighting.resolved;
		data->output = bb.lighting.resolved;

		R_Pass *pass = R_GraphAdd(g, String8Lit("Post Processing"), R_PassType_Compute);
		bb.lighting.resolved = R_PassWriteTextureCompute(pass, bb.lighting.resolved);
		R_PassReadTextureCompute(pass, bb.lighting.resolved);
		R_PassSetRecord(pass, R_PostProcessingPassFn, data);
	}

	R_DebugRendererRender(&s->debug_renderer, f->dt, g, f->arena, bb.lighting.resolved, bb.depth.resolved);
	
	R_GraphSetBackbuffer(g, bb.lighting.resolved);
}

internal void
R_SystemHotLoad(R_System *s)
{
	R_DebugRendererSelect(&s->debug_renderer);
}

/*
  R_TextureInfo swapchain_attachment_info = R_TextureInfoInit();
  swapchain_attachment_info.format = VK_FORMAT_R32G32B32A32_SFLOAT;
  R_GraphTexHandle dummy_handle = R_GraphCreateTexture(&app->graph, &swapchain_attachment_info);
  R_Clear clear = R_ClearColour((f32)I_KbDown(input, I_KeyboardKey_Tab), 0.0f, 0.0f, 1.f);
  R_Pass *dummy = R_GraphAdd(&app->graph, String8Lit("dummy"), R_PassType_Graphics);
  R_PassWriteColour(dummy, dummy_handle, &clear);
  R_GraphSetBackbuffer(&app->graph, dummy_handle);
*/
