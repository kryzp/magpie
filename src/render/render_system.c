
static R_System *r_system = NULL;

internal void R_SystemInitAndSelect(R_System *system, Arena *arena, LOG_Channel log_channel)
{
	system->arena = arena;
	system->log_channel = log_channel;
	
	G_BufferAllocInfo ring_buffer_alloc_info = {0};
	ring_buffer_alloc_info.size = R_FRAME_UPLOAD_BUDGET * G_FRAMES_IN_FLIGHT;
	ring_buffer_alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	ring_buffer_alloc_info.usage =
		VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT |
		VK_BUFFER_USAGE_2_TRANSFER_SRC_BIT |
		VK_BUFFER_USAGE_2_TRANSFER_DST_BIT;
	system->upload_ring = G_RingBufferAlloc(&ring_buffer_alloc_info);

	for (u32 i = 0; i < G_FRAMES_IN_FLIGHT; i++)
		system->upload_slices[i] = G_RingBufferSliceView(&system->upload_ring, i * R_FRAME_UPLOAD_BUDGET, R_FRAME_UPLOAD_BUDGET);
	
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
		
		system->shaders.debug_line_handle             = A_RequireAsset(String8Lit("assets://shaders/passes/debug/debug_line.slang"), counter);
		system->shaders.forward_handle                = A_RequireAsset(String8Lit("assets://shaders/passes/forward/forward.slang"), counter);
		system->shaders.shadow_handle                 = A_RequireAsset(String8Lit("assets://shaders/passes/shadow/shadow_mapping.slang"), counter);
		system->shaders.cull_frustum_handle           = A_RequireAsset(String8Lit("assets://shaders/passes/culling/frustum_culling.slang"), counter);
		system->shaders.cull_sphere_handle            = A_RequireAsset(String8Lit("assets://shaders/passes/culling/sphere_culling.slang"), counter);
		system->shaders.skybox_handle                 = A_RequireAsset(String8Lit("assets://shaders/passes/post/skybox.slang"), counter);
		system->shaders.tonemapping_handle            = A_RequireAsset(String8Lit("assets://shaders/passes/post/hdr_tonemapping.slang"), counter);
		system->shaders.brdf_lut_generation_handle    = A_RequireAsset(String8Lit("assets://shaders/passes/ibl/brdf_lut.slang"), counter);
		system->shaders.hdr_to_cubemap_handle         = A_RequireAsset(String8Lit("assets://shaders/passes/ibl/hdr_to_environment_cubemap.slang"), counter);
		system->shaders.irradiance_cubemap_gen_handle = A_RequireAsset(String8Lit("assets://shaders/passes/ibl/irradiance_convolution.slang"), counter);
		system->shaders.prefilter_cubemap_gen_handle  = A_RequireAsset(String8Lit("assets://shaders/passes/ibl/prefilter_convolution.slang"), counter);
		
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

	// IBL
	{
		system->brdf_lut = G_TextureAlloc2D(512, 512, VK_FORMAT_R32G32_SFLOAT, 1);
		
		system->environment_cubemap = G_TextureAllocCubemap(512, VK_FORMAT_R32G32B32A32_SFLOAT, 8);
		system->irradiance_cubemap  = G_TextureAllocCubemap( 32, VK_FORMAT_R32G32B32A32_SFLOAT, 1);
		system->prefilter_cubemap   = G_TextureAllocCubemap(128, VK_FORMAT_R32G32B32A32_SFLOAT, R_SYSTEM_PREFILTER_MIPS);
		
		system->environment_hdr_handle = A_RequireAssetBlocking(String8Lit("assets://environment_map_1.hdr"));
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

	G_RingBufferDestroy(&r_system->upload_ring);

	DebugLogI(r_system->log_channel, "Destroyed.");

	r_system = NULL;
}

internal void R_SystemSelectContext(R_System *system)
{
	r_system = system;
	
	R_DebugRendererSelect(&r_system->debug_renderer);
}

internal G_RingBuffer *R_SystemBeginFrameUploads(R_System *system, u32 frame_slot_index)
{
	G_RingBuffer *slice = &system->upload_slices[frame_slot_index];
	G_RingBufferReset(slice);
	return slice;
}

internal void R_SystemBakeLookupsAndMaps(R_Graph *graph, const G_Swapchain *swapchain)
{
	ScratchArena scratch = ScratchBegin(NULL, 0);

	R_FrameParams params = {0};
	params.arena = scratch.arena;
	params.brdf_lut = r_system->brdf_lut;
	params.irradiance_fallback_cubemap = r_system->irradiance_cubemap;
	params.prefilter_cubemap = r_system->prefilter_cubemap;
	params.skybox_mesh = &r_system->skybox_mesh;
	params.cubemap_capture_transform_buffer = r_system->cubemap_capture_transform_buffer;
	params.linear_sampler = r_system->samplers.linear;
	params.nearest_sampler = r_system->samplers.nearest;

	if (!R_FrameParamsResolveShaders(r_system, &params))
	{
		DebugLogE(r_system->log_channel, "IBL bake skipped: Shaders not loaded.");
		ScratchRelease(&scratch);
		return;
	}

	R_SystemGenerateLookupsAndMaps(graph, &params);
	R_GraphCompile(graph, swapchain);

	G_CmdBuffer cmd = G_SubmitImBegin();
	R_GraphExecute(graph, swapchain, &cmd);
	G_SubmitImEnd(&cmd);

	R_GraphReset(graph);

	ScratchRelease(&scratch);

	DebugLogI(r_system->log_channel, "Baked lookups and maps.");
}

internal void R_SystemGenerateLookupsAndMaps(R_Graph *graph, const R_FrameParams *frame_params)
{
	const u32 prefilter_mips = R_SYSTEM_PREFILTER_MIPS;
	Arena *pass_arena = frame_params->arena;
	
	G_ResourceKey hdr_texture_gfx = A_GetOrBreak(r_system->environment_hdr_handle)->texture.key;
	
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

/*
 * 1. CLEAR
 * 2. SHADOWS
 * 3. FRUSTUM CULL
 * 4. FORWARD - OPAQUE
 * 5. SKYBOX
 * 6. FORWARD - TRANSLUCENT
 * 7. POST PROCESSING
 * 8. DEBUG LINES
 * 9. PRESENT / BLIT
 */
internal void R_SystemRender(R_Graph *graph, const R_FrameParams *frame_params)
{
	R_FrustumVolume frustum = R_CameraFrustum(&frame_params->camera);

	R_Blackboard bb = {0};

	R_Clear colour_clear = R_ClearColour(0.f, 0.f, 0.f, 1.f);
	R_Clear depth_clear = R_ClearDepthStencil(1.f, 0);

	R_TextureInfo lighting_info = R_TextureInfoInitSwapchain(VK_FORMAT_R16G16B16A16_SFLOAT, v3x(1.f));
	lighting_info.flags = G_TextureAllocFlag_Storage;
	lighting_info.samples = VK_SAMPLE_COUNT_1_BIT;
	bb.lighting_resolve = R_GraphCreateTexture(graph, &lighting_info);
	lighting_info.flags = G_TextureAllocFlag_None;
	lighting_info.samples = VK_SAMPLE_COUNT_4_BIT;
	bb.lighting_msaa = R_GraphCreateTexture(graph, &lighting_info);

	R_TextureInfo depth_info = R_TextureInfoInitSwapchain(G_GetDepthStencilFormat(), v3x(1.f));
	depth_info.samples = VK_SAMPLE_COUNT_1_BIT;
	bb.depth_resolve = R_GraphCreateTexture(graph, &depth_info);
	depth_info.samples = VK_SAMPLE_COUNT_4_BIT;
	bb.depth_msaa = R_GraphCreateTexture(graph, &depth_info);

	// todo: fold the clear into the first pass that writes each target.
	R_Pass *clear_pass = R_GraphAdd(graph, String8Lit("Clear"), R_PassType_Graphics);
	bb.lighting_msaa = R_PassWriteColour(clear_pass, bb.lighting_msaa, &colour_clear);
	bb.depth_msaa = R_PassWriteDepth(clear_pass, bb.depth_msaa, &depth_clear);

	// shadows + forward opaque
	if (frame_params->object_count > 0)
	{
		R_ShadowsRender(&r_system->shadow_render_state, graph, frame_params, &bb);

		R_DrawStream draw_stream = R_CullFrustum(graph, frame_params, R_CullFilter_OpaqueOnly, &frustum);

		R_ForwardRender(graph, frame_params, &bb, &draw_stream);
	}

	// skybox
	{
		R_SkyboxPassData *data = ArenaPushArray(frame_params->arena, R_SkyboxPassData, 1);
		data->frame_params = frame_params;
		data->cubemap = G_TextureViewAuto(r_system->environment_cubemap);

		R_Pass *skybox_pass = R_GraphAdd(graph, String8Lit("Skybox"), R_PassType_Graphics);
		bb.lighting_msaa = R_PassWriteColour(skybox_pass, bb.lighting_msaa, NULL);
		bb.depth_msaa = R_PassWriteDepth(skybox_pass, bb.depth_msaa, NULL);
		R_PassReadTextureGraphics(skybox_pass, R_GraphImportTexture(graph, r_system->environment_cubemap));
		R_PassSetRecord(skybox_pass, R_SkyboxPassFn, data);
	}

	// forward translucent + MSAA resolve
	R_ForwardRenderTranslucent(graph, frame_params, &bb);

	// post processing
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
