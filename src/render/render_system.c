
static void R_SystemCreateSkyboxMesh(R_System *s)
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

static void R_SystemInit(R_System *s, Arena *arena, G_Device *device, A_Assets *assets, LOG_Channel log_channel)
{
	s->arena = arena;
	s->device = device;
	s->assets = assets;
	s->log_channel = log_channel;
	
	G_BufferAllocInfo ring_buffer_alloc_info = {0};
	ring_buffer_alloc_info.size = Megabytes(512);
	ring_buffer_alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	ring_buffer_alloc_info.usage =
		VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT |
		VK_BUFFER_USAGE_2_TRANSFER_SRC_BIT |
		VK_BUFFER_USAGE_2_TRANSFER_DST_BIT;
	s->frame_upload_ring_buffer = G_RingBufferAlloc(s->device, &ring_buffer_alloc_info);
	
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

	//R_ShadowStateInit(&s->shadow_state, s->device);
	R_DebugRendererInitAndSelect(&s->debug_renderer, s->arena, s->device, s->assets);
	
	DebugLogI(s->log_channel, "Initialized.");
}

static void R_SystemDestroy(R_System *s)
{
	//R_IrradianceVolumeDestroy(&s->irradiance_volume);
	R_DebugRendererDestroy(&s->debug_renderer);
	//R_ShadowStateFree(&s->shadow_state);

	G_DeviceTextureDestroy(s->device, s->brdf_lut);
	G_DeviceTextureDestroy(s->device, s->environment_cubemap);
	G_DeviceTextureDestroy(s->device, s->irradiance_cubemap);
	G_DeviceTextureDestroy(s->device, s->prefilter_cubemap);
	
	R_MeshDestroy(&s->skybox_mesh, s->device);
	
	G_DeviceSamplerDestroy(s->device, s->linear_sampler);
	G_DeviceSamplerDestroy(s->device, s->nearest_sampler);

	G_DeviceBufferDestroy(s->device, s->cubemap_capture_transform_buffer);

	G_RingBufferDestroy(&s->frame_upload_ring_buffer, s->device);

	DebugLogI(s->log_channel, "Destroyed.");
}

static void R_SystemGenerateLookupsAndMaps(R_System *s, R_Graph *g, Arena *arena)
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
	A_Handle hdr_texture_handle        = A_Require(s->assets, String8Lit("assets://environment_map_3.hdr"),                               A_Type_Texture);
	
	G_ShaderKey brdf_lut_shader        = A_GetNow(s->assets, brdf_lut_shader_handle)->shader.key;
	G_ShaderKey hdr_to_env_shader      = A_GetNow(s->assets, hdr_to_env_shader_handle)->shader.key;
	G_ShaderKey irradiance_pass_shader = A_GetNow(s->assets, irradiance_shader_handle)->shader.key;
	G_ShaderKey prefilter_pass_shader  = A_GetNow(s->assets, prefilter_shader_handle)->shader.key;
	G_TextureKey hdr_texture_gfx       = A_GetNow(s->assets, hdr_texture_handle)->texture.key;

	// Generate BRDF Lookup Table.
	/*
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
	*/

	/*
	R_IrradianceVolumeInit(&s->irradiance_volume,
						   s->device, s->assets,
						   osapi->LogChannelOpenFrom(s->log_channel, String8Lit("IRRADIANCE")),
						   v3(-8.f, -6.f,  -1.f),
						   v3( 8.f,  6.f,  12.f),
						   1, 1, 1,
						   &s->skybox_mesh,
						   G_DeviceTextureViewAuto(s->device, s->environment_cubemap),
						   s->linear_sampler);
	*/

	DebugLogI(s->log_channel, "Generated lookups and maps.");
}

static void R_SystemRender(R_System *s, R_Graph *graph, const R_FrameParams *frame_params)
{
	const R_Camera *camera = &frame_params->camera;

	R_FrustumVolume frustum = R_CameraFrustum(camera);
	
	u32 window_width, window_height;
	osapi->GetWindowSize(&window_width, &window_height);
	
	/*
	R_GPU_FrameData frame_data = {0};
	frame_data.view = camera->view;
	frame_data.proj = camera->proj;
	frame_data.view_proj = camera->view_proj;
	frame_data.view_proj_no_translation = camera->view_proj_no_translation;
	frame_data.inv_view = camera->inv_view;
	frame_data.inv_proj = camera->inv_proj;
	frame_data.camera_position = camera->position;
	frame_data.window_resolution = v2(window_width, window_height);
	frame_data.delta_time = frame_params->dt;
	frame_data.time = frame_params->elapsed;

	G_DeviceBufferWrite(s->device,
						s->frame_data_buffer,
						&frame_data, sizeof(frame_data), 0);
						*/

	R_Blackboard bb = {0};

	R_TextureInfo lighting_info = R_TextureInfoInitSwapchain(VK_FORMAT_R16G16B16A16_SFLOAT, v3(1.f, 1.f, 1.f));
	lighting_info.flags = G_TextureAllocFlag_Storage;
	bb.lighting = R_GraphCreateMsaa(graph, &lighting_info, VK_SAMPLE_COUNT_4_BIT);
	R_Clear colour_clear = R_ClearColour(0.f, 0.f, 0.f, 1.f);

	R_TextureInfo depth_info = R_TextureInfoInitSwapchain(graph->device->context.depth_format, v3(1.f, 1.f, 1.f));
	bb.depth = R_GraphCreateMsaa(graph, &depth_info, VK_SAMPLE_COUNT_4_BIT);
	R_Clear depth_clear = R_ClearDepthStencil(1.f, 0);

	R_Pass *clear_pass = R_GraphAdd(graph, String8Lit("Clear"), R_PassType_Graphics);
	bb.lighting.msaa = R_PassWriteColour(clear_pass, bb.lighting.msaa, &colour_clear);
	bb.depth.msaa = R_PassWriteDepth(clear_pass, bb.depth.msaa, &depth_clear);

	/*
	if (f->scene_data.object_count > 0)
	{
		R_ShadowRendererUploadGPU(&s->shadow_renderer, &bt);

		R_ShadowRendererRender(&s->shadow_renderer, g, &bt, &bb, &s->culling);

		R_DrawStream draw_stream = R_CullFrustum(&s->culling, g, &bt, R_CullFilter_OpaqueOnly, &frustum);

		R_ForwardRender(g, s->assets, &frame_params, &bb, &draw_stream);
	}

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

		R_Pass *skybox_pass = R_GraphAdd(g, String8Lit("Skybox"), R_PassType_Graphics);
		bb.lighting.resolved = R_PassWriteColourResolve(skybox_pass, bb.lighting.msaa, bb.lighting.resolved, NULL);
		bb.depth.resolved = R_PassWriteDepthResolve(skybox_pass, bb.depth.msaa, bb.depth.resolved, NULL);
		R_PassReadTextureGraphics(skybox_pass, R_GraphImportTexture(g, s->environment_cubemap));
		R_PassSetRecord(skybox_pass, R_SkyboxPassFn, data);
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

		R_Pass *pp_pass = R_GraphAdd(g, String8Lit("Post Processing"), R_PassType_Compute);
		bb.lighting.resolved = R_PassWriteTextureCompute(pp_pass, bb.lighting.resolved);
		R_PassReadTextureCompute(pp_pass, bb.lighting.resolved);
		R_PassSetRecord(pp_pass, R_PostProcessingPassFn, data);
	}
	*/

	R_DebugRendererRender(&s->debug_renderer, graph, frame_params, bb.lighting.resolved, bb.depth.resolved);
	
	R_GraphSetBackbuffer(graph, bb.lighting.resolved);
	R_GraphSetPresentFilter(graph, VK_FILTER_LINEAR);
}

static void R_SystemHotLoad(R_System *s)
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
