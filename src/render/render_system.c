
static R_System *r_system = NULL;

internal void R_SystemInitAndSelect(R_System *system, Arena *arena, LOG_Channel log_channel)
{
	system->arena = arena;
	system->log_channel = log_channel;
	
	G_BufferAllocInfo ring_buffer_alloc_info = {0};
	ring_buffer_alloc_info.size = Megabytes(512);
	ring_buffer_alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	ring_buffer_alloc_info.usage =
		VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT |
		VK_BUFFER_USAGE_2_TRANSFER_SRC_BIT |
		VK_BUFFER_USAGE_2_TRANSFER_DST_BIT;
	system->frame_upload_ring_buffer = G_RingBufferAlloc(&ring_buffer_alloc_info);
	
	m4 capture_view_matrices[] = {
		M4LookAt(v3(0.f, 0.f, 0.f), v3( 1.f, 0.f, 0.f), v3( 0.f, 0.f, 1.f)), // +x :: right
		M4LookAt(v3(0.f, 0.f, 0.f), v3(-1.f, 0.f, 0.f), v3( 0.f, 0.f, 1.f)), // -x :: left
		M4LookAt(v3(0.f, 0.f, 0.f), v3( 0.f, 0.f, 1.f), v3( 0.f,-1.f, 0.f)), // +z :: up
		M4LookAt(v3(0.f, 0.f, 0.f), v3( 0.f, 0.f,-1.f), v3( 0.f, 1.f, 0.f)), // -z :: down
		M4LookAt(v3(0.f, 0.f, 0.f), v3( 0.f, 1.f, 0.f), v3( 0.f, 0.f, 1.f)), // +y :: forward
		M4LookAt(v3(0.f, 0.f, 0.f), v3( 0.f,-1.f, 0.f), v3( 0.f, 0.f, 1.f)), // -y :: backward
	};

	m4 capture_projection_matrix = M4Perspective(90.f, 1.f, 0.1f, 10.f);

	for (u32 i = 0; i < 6; i++)
		capture_view_matrices[i] = M4MulM4(capture_projection_matrix, capture_view_matrices[i]);

	G_BufferAllocInfo cubemap_capture_buffer_alloc_info = {0};
	cubemap_capture_buffer_alloc_info.usage = VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_2_TRANSFER_DST_BIT;
	cubemap_capture_buffer_alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	cubemap_capture_buffer_alloc_info.size = sizeof(capture_view_matrices);

	system->cubemap_capture_transform_buffer = G_BufferAlloc(&cubemap_capture_buffer_alloc_info);
	
	G_BufferWrite(system->cubemap_capture_transform_buffer,
						capture_view_matrices,
						sizeof(capture_view_matrices), 0);

	// Init shaders.
	{
		OS_Handle counter = osapi->JobCounterAlloc(0);
		
		system->shaders.debug_line_handle             = A_RequireAsset(system->arena, String8Lit("assets://shaders/passes/debug/debug_line.slang"), A_Type_Shader, counter);
		system->shaders.forward_handle                = A_RequireAsset(system->arena, String8Lit("assets://shaders/passes/forward/forward.slang"), A_Type_Shader, counter);
		system->shaders.shadow_handle                 = A_RequireAsset(system->arena, String8Lit("assets://shaders/passes/shadow/shadow_mapping.slang"), A_Type_Shader, counter);
		system->shaders.cull_frustum_handle           = A_RequireAsset(system->arena, String8Lit("assets://shaders/passes/culling/frustum_culling.slang"), A_Type_Shader, counter);
		system->shaders.cull_sphere_handle            = A_RequireAsset(system->arena, String8Lit("assets://shaders/passes/culling/sphere_culling.slang"), A_Type_Shader, counter);
		system->shaders.skybox_handle                 = A_RequireAsset(system->arena, String8Lit("assets://shaders/passes/post/skybox.slang"), A_Type_Shader, counter);
		system->shaders.tonemapping_handle            = A_RequireAsset(system->arena, String8Lit("assets://shaders/passes/post/hdr_tonemapping.slang"), A_Type_Shader, counter);
		system->shaders.brdf_lut_generation_handle    = A_RequireAsset(system->arena, String8Lit("assets://shaders/passes/ibl/brdf_lut.slang"), A_Type_Shader, counter);
		system->shaders.hdr_to_cubemap_handle         = A_RequireAsset(system->arena, String8Lit("assets://shaders/passes/ibl/hdr_to_environment_cubemap.slang"), A_Type_Shader, counter);
		system->shaders.irradiance_cubemap_gen_handle = A_RequireAsset(system->arena, String8Lit("assets://shaders/passes/ibl/irradiance_convolution.slang"), A_Type_Shader, counter);
		system->shaders.prefilter_cubemap_gen_handle  = A_RequireAsset(system->arena, String8Lit("assets://shaders/passes/ibl/prefilter_convolution.slang"), A_Type_Shader, counter);
		
		A_WaitForLoadAndRelease(counter);
	}
	
	// Init samplers.
	{
		system->samplers.linear = G_SamplerCreateF(VK_FILTER_LINEAR);
		system->samplers.nearest = G_SamplerCreateF(VK_FILTER_NEAREST);		
	}
	
	// Init skybox mesh.
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
	
		R_MeshAlloc(&system->skybox_mesh,
					sizeof(v3), VK_INDEX_TYPE_UINT16,
					ArraySize(vertices), ArraySize(indices));

		G_ResourceKey staging_buffer = G_StageAlloc(R_MeshVertexBufferSize(&system->skybox_mesh) + R_MeshIndexBufferSize(&system->skybox_mesh));

		R_MeshWriteToStage(&system->skybox_mesh,
						   staging_buffer, 0,
						   vertices, indices);

		{
			G_CmdBuffer cmd = G_SubmitImBegin();
			R_MeshUpload(&system->skybox_mesh, &cmd, staging_buffer, 0);
			G_SubmitImEnd(&cmd);
		}

		G_BufferDestroy(staging_buffer);
	}

	// Init sub-render systems.
	{
		R_ShadowsInit(&system->shadow_render_state);
		R_DebugRendererInitAndSelect(&system->debug_renderer, system->arena);
	}

	R_SystemSelectContext(system);
	
	DebugLogI(system->log_channel, "Initialized.");
}

internal void R_SystemDestroy(void)
{
	//R_IrradianceVolumeDestroy(&r_system->irradiance_volume);
	R_DebugRendererDestroy();
	R_ShadowsDestroy(&r_system->shadow_render_state);

	G_TextureDestroy(r_system->brdf_lut);
	G_TextureDestroy(r_system->environment_cubemap);
	G_TextureDestroy(r_system->irradiance_cubemap);
	G_TextureDestroy(r_system->prefilter_cubemap);
	
	R_MeshDestroy(&r_system->skybox_mesh);
	
	G_SamplerDestroy(r_system->samplers.linear);
	G_SamplerDestroy(r_system->samplers.nearest);

	G_BufferDestroy(r_system->cubemap_capture_transform_buffer);

	G_RingBufferDestroy(&r_system->frame_upload_ring_buffer);

	DebugLogI(r_system->log_channel, "Destroyed.");

	r_system = NULL;
}

internal void R_SystemSelectContext(R_System *system)
{
	r_system = system;
	
	R_DebugRendererSelect(&r_system->debug_renderer);
}

internal void R_SystemGenerateLookupsAndMaps(R_Graph *graph, Arena *pass_arena, const R_FrameParams *frame_params)
{
	const u32 prefilter_mips = 5;

	r_system->brdf_lut = G_TextureAlloc2D(512, 512, VK_FORMAT_R32G32_SFLOAT, 1);
	
	r_system->environment_cubemap = G_TextureAllocCubemap(512, VK_FORMAT_R32G32B32A32_SFLOAT, 8);
	r_system->irradiance_cubemap  = G_TextureAllocCubemap( 32, VK_FORMAT_R32G32B32A32_SFLOAT, 1);
	r_system->prefilter_cubemap   = G_TextureAllocCubemap(128, VK_FORMAT_R32G32B32A32_SFLOAT, prefilter_mips);
	
	A_Handle hdr_texture_handle = A_RequireAssetBlocking(r_system->arena, String8Lit("assets://environment_map_1.hdr"), A_Type_Texture);
	G_ResourceKey hdr_texture_gfx = A_GetOrBreak(hdr_texture_handle)->texture.key;
	
	// Generate BRDF Lookup Table.
	{
		R_BRDFLutPassData *data = ArenaPushArray(pass_arena, R_BRDFLutPassData, 1);
		data->frame_params = frame_params;
		
		R_Pass *pass = R_GraphAdd(graph, String8Lit("BRDF LUT"), R_PassType_Graphics);
		R_PassSetRecord(pass, R_BRDFLutPassFn, data);
		R_PassWriteColour(pass, R_GraphImportTexture(graph, r_system->brdf_lut), NULL);
	}

	// Generate Environment Cubemap.
	{
		R_HdrToEnvPassData *data = ArenaPushArray(pass_arena, R_HdrToEnvPassData, 1);
		data->frame_params = frame_params;
		data->hdr_view = G_TextureViewAuto(hdr_texture_gfx);
		
		R_Pass *pass = R_GraphAdd(graph, String8Lit("HDR -> Environment Map"), R_PassType_Graphics);
		R_PassSetRecord(pass, R_HdrToEnvPassFn, data);
		R_PassSetMultiViewMask(pass, 0b111111);
		R_PassWriteColour(pass, R_GraphImportTexture(graph, r_system->environment_cubemap), NULL);

		R_GenerateMipsPassData *mips_data = ArenaPushArray(pass_arena, R_GenerateMipsPassData, 1);
		mips_data->texture = r_system->environment_cubemap;
		
		R_Pass *pass_mipmaps = R_GraphAdd(graph, String8Lit("Environment Map Mipmapping"), R_PassType_Transfer);
		R_PassSetRecord(pass_mipmaps, R_GenerateMipsPassFn, mips_data);
		R_PassBlitTextureDst(pass_mipmaps, R_GraphImportTexture(graph, r_system->environment_cubemap));
	}
	
	// Irradiance.
	{
		R_IBLPassIrradianceData *data = ArenaPushArray(pass_arena, R_IBLPassIrradianceData, 1);
		data->frame_params = frame_params;
		data->env_view = G_TextureViewAuto(r_system->environment_cubemap);
		
		R_Pass *pass = R_GraphAdd(graph, String8Lit("Irradiance"), R_PassType_Graphics);
		R_PassSetRecord(pass, R_IBLPassIrradianceFn, data);
		R_PassSetMultiViewMask(pass, 0b111111);
		R_PassWriteColour(pass, R_GraphImportTexture(graph, r_system->irradiance_cubemap), NULL);
	}
	
	// Prefilter.
	{
		const u32 mipmap_count = prefilter_mips;
		
		for (u32 i = 0; i < mipmap_count; i++)
		{
			R_IBLPassPrefilterData *data = ArenaPushArray(pass_arena, R_IBLPassPrefilterData, 1);
			data->frame_params = frame_params;
			data->env_view = G_TextureViewAuto(r_system->environment_cubemap);
			data->roughness = (f32)i / (f32)(mipmap_count - 1);

			G_SubresourceRange range = {0};
			range.aspects = VK_IMAGE_ASPECT_COLOR_BIT;
			range.base_mip = i;
			range.mips = 1;
			range.base_layer = 0;
			range.layers = 6;
		
			R_Pass *pass = R_GraphAdd(graph, String8Lit("Prefilter"), R_PassType_Graphics);
			R_PassSetRecord(pass, R_IBLPassPrefilterFn, data);
			R_PassSetMultiViewMask(pass, 0b111111);
			R_PassWriteColourEx(pass, R_GraphImportTexture(graph, r_system->prefilter_cubemap), NULL, range);
		}
	}

	/*
	R_IrradianceVolumeInit(&r_system->irradiance_volume,
						   r_system->device, r_system->assets,
						   osapi->LogChannelOpenFrom(r_system->log_channel, String8Lit("IRRADIANCE")),
						   v3(-8.f, -6.f,  -1.f),
						   v3( 8.f,  6.f,  12.f),
						   1, 1, 1,
						   &r_system->skybox_mesh,
						   G_TextureViewAuto(r_system->device, r_system->environment_cubemap),
						   r_system->linear_sampler);
	*/

	DebugLogI(r_system->log_channel, "Generated lookups and maps.");
}

internal void R_SystemRender(R_Graph *graph, const R_FrameParams *frame_params)
{
	R_FrustumVolume frustum = R_CameraFrustum(&frame_params->camera);

	R_Blackboard bb = {0};

	R_Clear colour_clear = R_ClearColour(0.f, 0.f, 0.f, 1.f);
	R_Clear depth_clear = R_ClearDepthStencil(1.f, 0);

	R_TextureInfo lighting_info = R_TextureInfoInitSwapchain(VK_FORMAT_R16G16B16A16_SFLOAT, v3(0.5f, 0.5f, 1.0f));
	lighting_info.flags = G_TextureAllocFlag_Storage;
	lighting_info.samples = VK_SAMPLE_COUNT_1_BIT;
	bb.lighting_resolve = R_GraphCreateTexture(graph, &lighting_info);
	lighting_info.flags = G_TextureAllocFlag_None;
	lighting_info.samples = VK_SAMPLE_COUNT_4_BIT;
	bb.lighting_msaa = R_GraphCreateTexture(graph, &lighting_info);

	R_TextureInfo depth_info = R_TextureInfoInitSwapchain(G_GetDepthFormat(), v3(0.5f, 0.5f, 1.0f));
	depth_info.samples = VK_SAMPLE_COUNT_1_BIT;
	bb.depth_resolve = R_GraphCreateTexture(graph, &depth_info);
	depth_info.samples = VK_SAMPLE_COUNT_4_BIT;
	bb.depth_msaa = R_GraphCreateTexture(graph, &depth_info);

	// todo: this is ass
	R_Pass *clear_pass = R_GraphAdd(graph, String8Lit("Clear"), R_PassType_Graphics);
	bb.lighting_msaa = R_PassWriteColour(clear_pass, bb.lighting_msaa, &colour_clear);
	bb.depth_msaa = R_PassWriteDepth(clear_pass, bb.depth_msaa, &depth_clear);

	// Scene.
	if (frame_params->object_count > 0)
	{
		R_ShadowsUploadGPU(&r_system->shadow_render_state, frame_params);

		R_ShadowsRender(&r_system->shadow_render_state, graph, frame_params, &bb);

		R_DrawStream draw_stream = R_CullFrustum(graph, frame_params, R_CullFilter_OpaqueOnly, &frustum);

		R_ForwardRender(graph, frame_params, &bb, &draw_stream);
	}

	// Skybox.
	{
		R_SkyboxPassData *data = ArenaPushArray(frame_params->arena, R_SkyboxPassData, 1);
		data->frame_params = frame_params;
		data->cubemap = G_TextureViewAuto(r_system->environment_cubemap);

		R_Pass *skybox_pass = R_GraphAdd(graph, String8Lit("Skybox"), R_PassType_Graphics);
		bb.lighting_resolve = R_PassWriteColourResolve(skybox_pass, bb.lighting_msaa, bb.lighting_resolve, NULL);
		bb.depth_resolve = R_PassWriteDepthResolve(skybox_pass, bb.depth_msaa, bb.depth_resolve, NULL);
		R_PassReadTextureGraphics(skybox_pass, R_GraphImportTexture(graph, r_system->environment_cubemap));
		R_PassSetRecord(skybox_pass, R_SkyboxPassFn, data);
	}
	
	// Post Processing.
	{
		R_PostProcessingPassData *data = ArenaPushArray(frame_params->arena, R_PostProcessingPassData, 1);
		data->frame_params = frame_params;
		data->exposure = 0.5f;
		data->input = bb.lighting_resolve;
		data->output = bb.lighting_resolve;

		R_Pass *pp_pass = R_GraphAdd(graph, String8Lit("Post Processing"), R_PassType_Compute);
		bb.lighting_resolve = R_PassWriteTextureCompute(pp_pass, bb.lighting_resolve);
		R_PassReadTextureCompute(pp_pass, bb.lighting_resolve);
		R_PassSetRecord(pp_pass, R_PostProcessingPassFn, data);
	}

	R_DebugRendererRender(graph, frame_params, bb.lighting_resolve, bb.depth_resolve);
	
	R_GraphSetBackbuffer(graph, bb.lighting_resolve);
	R_GraphSetPresentFilter(graph, VK_FILTER_NEAREST);
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
