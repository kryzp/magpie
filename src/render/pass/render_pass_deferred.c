
// https://songho.ca/opengl/gl_sphere.html
// TODO: Use an icosphere or cubesphere for better tessellation.

/*
internal void
R_DeferredCreateLightSphereMesh(R_DeferredRenderer *dr)
{
	ScratchArena scratch = ScratchBegin(NULL, 0);

	u32 sector_count = 10;
	u32 stack_count  = 10;

	f32 sector_step = 2.f * MATH_PI / (f32)sector_count;
	f32 stack_step  =       MATH_PI / (f32)stack_count;

	u32 vertex_count = (stack_count + 1) * (sector_count + 1);
	u32 index_count  = (stack_count - 1) * (sector_count + 0) * 6;

	v3  *vertices = ArenaPushArray(scratch.arena, v3,  vertex_count);
	u16 *indices  = ArenaPushArray(scratch.arena, u16, index_count);

	u32 v_idx = 0;

	for (u32 i = 0; i <= stack_count; i++)
	{
		f32 theta = MATH_PI * 0.5f - (f32)i * stack_step;

		for (u32 j = 0; j <= sector_count; j++)
		{
			f32 phi = (f32)j * sector_step;
			vertices[v_idx++] = V3SphericalToCartesian(1.f, phi, theta);
		}
	}

	u32 i_idx = 0;

	for (u16 i = 0; i < (u16)stack_count; i++)
	{
		u16 k1 = (u16)(sector_count + 1) * (i + 0);
		u16 k2 = (u16)(sector_count + 1) * (i + 1);

		for (u16 j = 0; j < (u16)sector_count; j++, k1++, k2++)
		{
			if (i != 0)
			{
				indices[i_idx + 0] = k2;
				indices[i_idx + 1] = k1 + 1;
				indices[i_idx + 2] = k1;
				i_idx += 3;
			}

			if (i != (u16)stack_count - 1)
			{
				indices[i_idx + 0] = k2;
				indices[i_idx + 1] = k2 + 1;
				indices[i_idx + 2] = k1 + 1;
				i_idx += 3;
			}
		}
	}

	R_MeshAlloc(&dr->light_sphere_mesh, dr->device,
				sizeof(v3), VK_INDEX_TYPE_UINT16,
				vertex_count, index_count);

	G_BufferKey staging = G_DeviceStageAlloc(dr->device,
												 R_MeshVertexBufferSize(&dr->light_sphere_mesh) +
												 R_MeshIndexBufferSize(&dr->light_sphere_mesh));

	R_MeshWriteToStage(&dr->light_sphere_mesh, dr->device,
					   staging, 0, vertices, indices);

	G_CmdBuffer cmd = G_DeviceSubmitImBegin(dr->device);
	R_MeshUpload(&dr->light_sphere_mesh, &cmd, staging, 0);
	G_DeviceSubmitImEnd(dr->device, &cmd);

	G_DeviceBufferDestroy(dr->device, staging);

	ScratchRelease(&scratch);
}

R_PASS_RECORD_DEF(R_DeferredGeometryPassFn)
{
	G_Device *device = ctx->device;
	G_CmdBuffer *cmd = ctx->cmd;
	const R_Scene *scene = ctx->scene;

	const R_DeferredGeometryPassData *data = ctx->user_data;

	G_GraphicsPipelineDef pipeline_def = G_GraphicsPipelineDefInit(data->shader);
	pipeline_def.has_depth_attachment = true;
	pipeline_def.depth_stencil_state.depth_test_enabled = true;
	pipeline_def.depth_stencil_state.depth_write_enabled = true;

	for (u32 i = 0; i < R_GBufferAttachment_COUNT; i++)
		pipeline_def.colour_attachment_formats[pipeline_def.colour_attachment_count++] = VK_FORMAT_R32G32B32A32_SFLOAT;

	G_PipelineSt pipeline_st = G_DeviceFetchGraphicsPipeline(device, &pipeline_def);

	G_CmdBindBindless (cmd, VK_SHADER_STAGE_ALL_GRAPHICS, pipeline_st.layout);
	G_CmdBindPipeline (cmd, pipeline_st.bind_point, pipeline_st.pipeline);

	struct
	{
		u64 frame_data_buffer;
		u64 object_buffer;
		u64 material_buffer;
		u64 mesh_buffer;
		u32 sampler;
	}
	args;

	args.frame_data_buffer = G_DeviceBufferAddress(device, data->frame_data_buffer);
	args.object_buffer     = data->object_buffer_address;
	args.material_buffer   = R_SceneMaterialBufferAddress(scene);
	args.mesh_buffer       = R_SceneMeshBufferAddress(scene);
	args.sampler           = G_DeviceSamplerBindless(device, data->sampler);

	G_CmdPushConstants(cmd, pipeline_st.layout, VK_SHADER_STAGE_ALL_GRAPHICS, sizeof(args), &args, 0);

	G_BufferKey indirect_key = R_GraphResolveBuffer(ctx->graph, data->draw_stream.indirect_buffer);
	G_BufferKey counter_key  = R_GraphResolveBuffer(ctx->graph, data->draw_stream.count_buffer);

	R_SceneDrawIndirect(scene, cmd, indirect_key, counter_key);
}

internal void
R_DeferredRendererInit(R_DeferredRenderer *dr, G_Device *device, A_Registry *assets)
{
	dr->device = device;
	dr->assets = assets;

	dr->model_shader                 = A_Require(assets, String8Lit("assets://shaders/passes/geometry/model.slang"),                 A_Type_Shader);
	dr->ambient_lighting_shader      = A_Require(assets, String8Lit("assets://shaders/passes/lighting/ambient_lighting.slang"),      A_Type_Shader);
	dr->direct_lighting_point_shader = A_Require(assets, String8Lit("assets://shaders/passes/lighting/direct_lighting_point.slang"), A_Type_Shader);

	R_DeferredCreateLightSphereMesh(dr);
}

internal void
R_DeferredRendererDestroy(R_DeferredRenderer *dr)
{
	R_MeshDestroy(&dr->light_sphere_mesh, dr->device);
}

internal void
R_DeferredRenderGeometry(R_DeferredRenderer *dr,
						 R_Graph *graph,
						 R_Blackboard *bb,
						 Arena *pass_arena,
						 const R_SceneResources *scene_resources,
						 G_BufferKey frame_data_buffer,
						 G_SamplerKey linear_sampler,
						 const R_DrawStream *draw_stream)
{
	R_BB_GBufferData *gbuffer = &bb->gbuffer;

	R_Clear depth_clear  = R_ClearDepthStencil(1.f, 0);
	R_Clear colour_clear = R_ClearColour(0.f, 0.f, 0.f, 1.f);

	R_TextureInfo depth_info = R_TextureInfoInitDepth(graph->device);

	R_TextureInfo colour_info = R_TextureInfoInit();
	colour_info.format = VK_FORMAT_R32G32B32A32_SFLOAT;

	R_Pass *pass = R_GraphAdd(graph, String8Lit("Geometry"), R_PassType_Graphics);

	for (u32 i = 0; i < R_GBufferAttachment_COUNT; i++)
	{
		gbuffer->attachments[i] = R_GraphCreateTexture(graph, &colour_info);
		gbuffer->attachments[i] = R_PassWriteColour(pass, gbuffer->attachments[i], &colour_clear);
	}

	gbuffer->depth = R_GraphCreateTexture(graph, &depth_info);
	gbuffer->depth = R_PassWriteDepth(pass, gbuffer->depth, &depth_clear);

	R_PassIndirectBuffer(pass, draw_stream->indirect_buffer);
	R_PassIndirectBuffer(pass, draw_stream->count_buffer);

	G_ShaderKey shader = A_ShaderDataGet(A_GetNow(dr->assets, dr->model_shader, A_Type_Shader));

	R_DeferredGeometryPassData *data = ArenaPushArray(pass_arena, R_DeferredGeometryPassData, 1);
	data->shader                = shader;
	data->frame_data_buffer     = frame_data_buffer;
	data->sampler               = linear_sampler;
	data->object_buffer_address = scene_resources->object_buffer.gpu;
	data->draw_stream           = *draw_stream;

	R_PassSetRecord(pass, R_DeferredGeometryPassFn, data);
}

R_PASS_RECORD_DEF(R_DeferredLightingPassFn)
{
	G_Device *device = ctx->device;
	G_CmdBuffer *cmd = ctx->cmd;
	const R_Scene *scene = ctx->scene;

	const R_DeferredLightingPassData *data = ctx->user_data;

	// -- Ambient Lighting
	{
		G_GraphicsPipelineDef pipeline_def = G_GraphicsPipelineDefInit(data->ambient_shader);
		pipeline_def.has_depth_attachment = true;
		pipeline_def.depth_stencil_state.depth_test_enabled  = false;
		pipeline_def.depth_stencil_state.depth_write_enabled = false;
		pipeline_def.colour_attachment_count = 1;
		pipeline_def.colour_attachment_formats[0] = VK_FORMAT_R32G32B32A32_SFLOAT;

		G_PipelineSt pipeline_st = G_DeviceFetchGraphicsPipeline(device, &pipeline_def);

		G_CmdBindBindless (cmd, VK_SHADER_STAGE_ALL_GRAPHICS, pipeline_st.layout);
		G_CmdBindPipeline (cmd, pipeline_st.bind_point, pipeline_st.pipeline);

		struct
		{
			u64 frame_data_buffer;

			u64 irradiance_sh_buffer;
			u64 irradiance_grid_info_buffer;
			
			u32 position;
			u32 albedo;
			u32 normal;
			u32 emissive;
			u32 material;

			u32 irradiance_fallback_cubemap;
			u32 prefilter_cubemap;
			u32 brdf_lut;

			u32 linear_sampler;
		}
		pc;

		pc.frame_data_buffer = G_DeviceBufferAddress(device, data->frame_data_buffer);

		pc.irradiance_sh_buffer        = data->irradiance_sh_buffer_address;
		pc.irradiance_grid_info_buffer = data->irradiance_grid_info_buffer_address;
		
		pc.position = G_DeviceTextureViewBindless(device, R_GraphResolveTextureView(ctx->graph, data->gbuffer.attachments[R_GBufferAttachment_Position],          G_SubresourceRangeAllColour()));
		pc.albedo   = G_DeviceTextureViewBindless(device, R_GraphResolveTextureView(ctx->graph, data->gbuffer.attachments[R_GBufferAttachment_Albedo],            G_SubresourceRangeAllColour()));
		pc.normal   = G_DeviceTextureViewBindless(device, R_GraphResolveTextureView(ctx->graph, data->gbuffer.attachments[R_GBufferAttachment_Normal],            G_SubresourceRangeAllColour()));
		pc.emissive = G_DeviceTextureViewBindless(device, R_GraphResolveTextureView(ctx->graph, data->gbuffer.attachments[R_GBufferAttachment_Emissive],          G_SubresourceRangeAllColour()));
		pc.material = G_DeviceTextureViewBindless(device, R_GraphResolveTextureView(ctx->graph, data->gbuffer.attachments[R_GBufferAttachment_MetallicRoughness], G_SubresourceRangeAllColour()));

		pc.irradiance_fallback_cubemap = G_DeviceTextureViewBindless(device, R_GraphResolveTextureView(ctx->graph, data->irradiance_fb_handle, G_SubresourceRangeAllColour()));
		pc.prefilter_cubemap           = G_DeviceTextureViewBindless(device, R_GraphResolveTextureView(ctx->graph, data->prefilter_handle,     G_SubresourceRangeAllColour()));
		pc.brdf_lut                    = G_DeviceTextureViewBindless(device, R_GraphResolveTextureView(ctx->graph, data->brdf_handle,          G_SubresourceRangeAllColour()));

		pc.linear_sampler = G_DeviceSamplerBindless(device, data->linear_sampler);

		G_CmdPushConstants(cmd, pipeline_st.layout, VK_SHADER_STAGE_ALL_GRAPHICS, sizeof(pc), &pc, 0);

		G_CmdDrawV(cmd, 3);
	}

	// -- Direct Point Lighting
	{
		G_GraphicsPipelineDef pipeline_def = G_GraphicsPipelineDefInit(data->direct_shader);
		pipeline_def.has_depth_attachment = true;
		pipeline_def.depth_stencil_state.depth_test_enabled  = false;
		pipeline_def.depth_stencil_state.depth_write_enabled = false;
		pipeline_def.cull_mode = VK_CULL_MODE_FRONT_BIT;
		pipeline_def.blend_state.enabled = true;
		pipeline_def.blend_state.colour.op  = VK_BLEND_OP_ADD;
		pipeline_def.blend_state.colour.src = VK_BLEND_FACTOR_ONE;
		pipeline_def.blend_state.colour.dst = VK_BLEND_FACTOR_ONE;
		pipeline_def.colour_attachment_count = 1;
		pipeline_def.colour_attachment_formats[0] = VK_FORMAT_R32G32B32A32_SFLOAT;

		G_PipelineSt pipeline_st = G_DeviceFetchGraphicsPipeline(device, &pipeline_def);

		G_CmdBindBindless (cmd, VK_SHADER_STAGE_ALL_GRAPHICS, pipeline_st.layout);
		G_CmdBindPipeline (cmd, pipeline_st.bind_point, pipeline_st.pipeline);

		R_MeshBind(data->light_sphere_mesh, cmd);

		u32 light_count = R_SceneGetLightCount(scene);

		// TODO: Instanced / Indirect rendering for lights.
		for (u32 i = 0; i < light_count; i++)
		{
			struct
			{
				u64 frame_data_buffer;
				u64 light_buffer;
				u64 vertex_buffer;
				u64 shadow_caster_buffer;

				u32 position;
				u32 albedo;
				u32 normal;
				u32 emissive;
				u32 material;

				u32 linear_sampler;
				u32 shadow_sampler;
			}
			pc;

			pc.frame_data_buffer = G_DeviceBufferAddress(device, data->frame_data_buffer);
			pc.light_buffer = data->light_buffer_address;
			pc.vertex_buffer = G_DeviceBufferAddress(device, data->light_sphere_mesh->vertex_buffer);
			pc.shadow_caster_buffer = G_DeviceBufferAddress(device, data->shadow_caster_table);

			pc.position = G_DeviceTextureViewBindless(device, R_GraphResolveTextureView(ctx->graph, data->gbuffer.attachments[R_GBufferAttachment_Position],          G_SubresourceRangeAllColour()));
			pc.albedo   = G_DeviceTextureViewBindless(device, R_GraphResolveTextureView(ctx->graph, data->gbuffer.attachments[R_GBufferAttachment_Albedo],            G_SubresourceRangeAllColour()));
			pc.normal   = G_DeviceTextureViewBindless(device, R_GraphResolveTextureView(ctx->graph, data->gbuffer.attachments[R_GBufferAttachment_Normal],            G_SubresourceRangeAllColour()));
			pc.emissive = G_DeviceTextureViewBindless(device, R_GraphResolveTextureView(ctx->graph, data->gbuffer.attachments[R_GBufferAttachment_Emissive],          G_SubresourceRangeAllColour()));
			pc.material = G_DeviceTextureViewBindless(device, R_GraphResolveTextureView(ctx->graph, data->gbuffer.attachments[R_GBufferAttachment_MetallicRoughness], G_SubresourceRangeAllColour()));

			pc.linear_sampler = G_DeviceSamplerBindless(device, data->linear_sampler);
			pc.shadow_sampler = G_DeviceSamplerBindless(device, data->linear_sampler); // TODO: use a nearest sampler for shadows.

			G_CmdPushConstants(cmd, pipeline_st.layout, VK_SHADER_STAGE_ALL_GRAPHICS, sizeof(pc), &pc, 0);

			R_MeshDrawInstanced(data->light_sphere_mesh, cmd, i);
		}
	}
}

internal R_GraphTexHandle
R_DeferredRenderLighting(R_DeferredRenderer *dr,
						 R_Graph *graph,
						 R_Blackboard *bb,
						 Arena *pass_arena,
						 const R_SceneResources *scene_resources,
						 G_BufferKey frame_data_buffer,
						 G_SamplerKey linear_sampler,
						 const R_IrradianceVolume *irradiance_volume,
						 G_TextureKey irradiance_fallback,
						 G_TextureKey prefilter,
						 G_TextureKey brdf)
{
	const R_BB_GBufferData *gbuffer = &bb->gbuffer;
	const R_BB_ShadowData  *shadow  = &bb->shadow_data;

	R_TextureInfo lighting_info = R_TextureInfoInit();
	lighting_info.format = VK_FORMAT_R32G32B32A32_SFLOAT;
	lighting_info.flags = G_TextureAllocFlag_Storage;
	
	R_GraphTexHandle lighting = R_GraphCreateTexture(graph, &lighting_info);

	R_Pass *pass = R_GraphAdd(graph, String8Lit("Lighting"), R_PassType_Graphics);

	lighting = R_PassWriteColour(pass, lighting, NULL);
	R_PassWriteDepth(pass, gbuffer->depth, NULL);

	for (u32 i = 0; i < R_GBufferAttachment_COUNT; i++)
		R_PassReadTextureGraphics(pass, gbuffer->attachments[i]);

	R_GraphTexHandle irradiance_fb_handle = R_PassReadTextureGraphics(pass, R_GraphImportTexture(graph, irradiance_fallback));
	R_GraphTexHandle prefilter_handle     = R_PassReadTextureGraphics(pass, R_GraphImportTexture(graph, prefilter));
	R_GraphTexHandle brdf_handle          = R_PassReadTextureGraphics(pass, R_GraphImportTexture(graph, brdf));

	for (u32 i = 0; i < shadow->shadow_map_count; i++)
		R_PassReadTextureGraphics(pass, shadow->shadow_maps[i]);

	G_ShaderKey ambient_shader = A_ShaderDataGet(A_GetNow(dr->assets, dr->ambient_lighting_shader,      A_Type_Shader));
	G_ShaderKey direct_shader  = A_ShaderDataGet(A_GetNow(dr->assets, dr->direct_lighting_point_shader, A_Type_Shader));

	R_DeferredLightingPassData *data = ArenaPushArray(pass_arena, R_DeferredLightingPassData, 1);
	data->ambient_shader       = ambient_shader;
	data->direct_shader        = direct_shader;
	data->frame_data_buffer    = frame_data_buffer;
	data->linear_sampler       = linear_sampler;
	data->light_buffer_address = scene_resources->light_buffer.gpu;
	data->shadow_caster_table  = shadow->shadow_caster_table;
	data->gbuffer              = *gbuffer;
	data->irradiance_fb_handle = irradiance_fb_handle;
	data->prefilter_handle     = prefilter_handle;
	data->brdf_handle          = brdf_handle;
	data->light_sphere_mesh    = &dr->light_sphere_mesh;

	if (R_IrradianceVolumeIsBaked(irradiance_volume))
	{
		data->irradiance_sh_buffer_address        = G_DeviceBufferAddress(graph->device, R_IrradianceVolumeGetSHBuffer(irradiance_volume));
		data->irradiance_grid_info_buffer_address = G_DeviceBufferAddress(graph->device, R_IrradianceVolumeGetGridInfoBuffer(irradiance_volume));
	}
	else
	{
		data->irradiance_sh_buffer_address = 0;
		data->irradiance_grid_info_buffer_address = 0;
	}
	
	R_PassSetRecord(pass, R_DeferredLightingPassFn, data);

	return lighting;
}
*/
