
internal RenderContextPerFrameData *
RenderContextGetCurrentFrame(RenderContext *context)
{
	return context->per_frame_data + graphics_device->current_frame_index;
}

internal void
RenderContextLoadShaders(RenderContext *context, MemoryArena *arena)
{
	struct
	{
		String8 vert;
		String8 frag;
		ShaderProgram *target;
	}
	graphics_shaders[] =
	{
		{ str8("res/brdf_lut_vertex.spv"),                    str8("res/brdf_lut_fragment.spv"),                    &context->brdf_lut_program },
		{ str8("res/hdr_to_environment_cubemap_vertex.spv"),  str8("res/hdr_to_environment_cubemap_fragment.spv"),  &context->hdr_to_environment_cubemap_program },
		{ str8("res/irradiance_convolution_vertex.spv"),      str8("res/irradiance_convolution_fragment.spv"),      &context->irradiance_map_program },
		{ str8("res/prefilter_convolution_vertex.spv"),       str8("res/prefilter_convolution_fragment.spv"),       &context->prefilter_map_program },
		{ str8("res/model_vertex.spv"),                       str8("res/model_fragment.spv"),                       &context->model_program },
		{ str8("res/skybox_vertex.spv"),                      str8("res/skybox_fragment.spv"),                      &context->skybox_program },
		{ str8("res/ambient_lighting_vertex.spv"),            str8("res/ambient_lighting_fragment.spv"),            &context->ambient_lighting_program },
		{ str8("res/direct_lighting_point_vertex.spv"),       str8("res/direct_lighting_point_fragment.spv") ,      &context->direct_lighting_point_program },
	};
	
	for(i32 i = 0; i < ArraySize(graphics_shaders); i++)
	{
		String8 files[] = { graphics_shaders[i].vert, graphics_shaders[i].frag };
		(*graphics_shaders[i].target) = ShaderProgramInit(arena, 2, files);
	}
}

// NOTE(kp): https://songho.ca/opengl/gl_sphere.html
// TODO(kp): Use a more efficient sphere shape like an ICOSPHERE or CUBESPHERE.
void RenderContextCreateUnitSphereMesh(RenderContext *context, MemoryArena *arena)
{
	ScratchArena scratch = GetScratch(arena, 1);
	
	u16 sector_count = 10;
	u16 stack_count = 10;
	
	f32 sector_step = 2.f * PIf / (f32)sector_count;
	f32 stack_step  =       PIf / (f32)stack_count;
	
	u32 vertex_count = (stack_count + 1) * (sector_count + 1);
	u32 index_count  = sector_count * (stack_count - 1) * 6;
	
	v3 *vertices = MemoryArenaPush(scratch.arena, sizeof(v3)  * vertex_count);
	u16 *indices = MemoryArenaPush(scratch.arena, sizeof(u16) * index_count);
	
	u32 index = 0;
	
	for(i32 i = 0; i <= stack_count; i++)
	{
		f32 theta = PIf/2.f - i*stack_step;
		
		for(i32 j = 0; j <= sector_count; j++)
		{
			f32 phi = j*sector_step;
			
			vertices[index++] = SphericalToCartesian(1.f, phi, theta);
		}
	}
	
	index = 0;
	
	for(u16 i = 0; i < stack_count; i++)
	{
		u16 k1 = i  * (sector_count + 1); // NOTE(kp): Current stack.
		u16 k2 = k1 + (sector_count + 1); // NOTE(kp): Next stack.
		
		for(u16 j = 0; j < sector_count; j++, k1++, k2++)
		{
			if(i != 0)
			{
				indices[index + 0] = k1;
				indices[index + 1] = k2;
				indices[index + 2] = k1 + 1u;
				
				index += 3;
			}
			
			if(i != stack_count-1)
			{
				indices[index + 0] = k1 + 1u;
				indices[index + 1] = k2;
				indices[index + 2] = k2 + 1u;
				
				index += 3;
			}
		}
	}
	
	context->light_sphere_mesh = MeshInit(&vertex_formats->v3_format,
										  vertex_count, vertices,
										  index_count, indices);
	
	ReleaseScratch(&scratch);
}

internal void
RenderContextCreatePerFrameObjects(RenderContext *context)
{
	for(i32 i = 0; i < FRAMES_IN_FLIGHT; i++)
	{
		RenderContextPerFrameData *frame = context->per_frame_data + i;
		
		frame->frame_data_buffer = GPUBufferAlloc(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
												  VK_BUFFER_USAGE_TRANSFER_DST_BIT,
												  VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
												  sizeof(GPU_FrameData));
		
		frame->object_buffer = GPUBufferAlloc(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
											  VK_BUFFER_USAGE_TRANSFER_DST_BIT,
											  VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
											  sizeof(GPU_ObjectData) * SCENE_MAX_OBJECTS);
		
		/*
		frame->light_buffer = GPUBufferAlloc(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
											 VK_BUFFER_USAGE_TRANSFER_DST_BIT,
											 VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
											 sizeof(GPU_Light) * ArraySize(renderer->lights));
		*/
		
		frame->indirect_buffer = GPUBufferAlloc(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
												VK_BUFFER_USAGE_TRANSFER_DST_BIT |
												VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
												VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
												sizeof(VkDrawIndexedIndirectCommand) * SCENE_MAX_OBJECTS);
	}
}

internal void
RenderContextDestroyPerFrameObjects(RenderContext *context)
{
	for(i32 i = 0; i < FRAMES_IN_FLIGHT; i++)
	{
		RenderContextPerFrameData *frame = context->per_frame_data + i;
		
		GPUBufferDestroy(&frame->frame_data_buffer);
		GPUBufferDestroy(&frame->object_buffer);
		//GPUBufferDestroy(&frame->light_buffer);
		GPUBufferDestroy(&frame->indirect_buffer);
	}
}

internal void
RenderContextInit(RenderContext *context, MemoryArena *arena)
{
	context->linear_sampler = SamplerInitFilter(VK_FILTER_NEAREST);
	
	m4 capture_projection_matrix = M4Perspective(90.f, 1.f, 0.1f, 10.f);
	
	// NOTE(kp): Renderman introduced the left-handed Y-up cubemap in 1990
	//           but we use right-handed Z-up so we have to flip these weirdly.
	m4 capture_view_matrices[] = {
		M4LookAt(v3(0.f, 0.f, 0.f), v3( 1.f, 0.f, 0.f), v3(0.f, 0.f, 1.f)), // X+
		M4LookAt(v3(0.f, 0.f, 0.f), v3(-1.f, 0.f, 0.f), v3(0.f, 0.f, 1.f)), // X-
		M4LookAt(v3(0.f, 0.f, 0.f), v3( 0.f, 0.f, 1.f), v3(0.f,-1.f, 0.f)), // Y+
		M4LookAt(v3(0.f, 0.f, 0.f), v3( 0.f, 0.f,-1.f), v3(0.f, 1.f, 0.f)), // Y-
		M4LookAt(v3(0.f, 0.f, 0.f), v3( 0.f, 1.f, 0.f), v3(0.f, 0.f, 1.f)), // Z+
		M4LookAt(v3(0.f, 0.f, 0.f), v3( 0.f,-1.f, 0.f), v3(0.f, 0.f, 1.f)), // Z-
	};
	
	context->cubemap_capture_transforms = GPUBufferAlloc(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
														 VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
														 sizeof(m4) * 6);
	
	for(i32 i = 0; i < 6; i++)
	{
		m4 m = M4MultiplyM4(capture_projection_matrix, capture_view_matrices[i]);
		GPUBufferWrite(&context->cubemap_capture_transforms, &m, sizeof(m4), sizeof(m4) * i);
	}
	
	v3 vertices[] = {
		{ -1.f,  1.f,  1.f },
		{ -1.f,  1.f, -1.f },
		{  1.f,  1.f, -1.f },
		{  1.f,  1.f,  1.f },
		{ -1.f, -1.f,  1.f },
		{ -1.f, -1.f, -1.f },
		{  1.f, -1.f, -1.f },
		{  1.f, -1.f,  1.f }
	};
	
	u16 indices[] = {
		0, 2, 1,
		2, 0, 3,
		
		7, 5, 6,
		5, 7, 4,
		
		4, 1, 5,
		1, 4, 0,
		
		3, 6, 2,
		6, 3, 7,
		
		1, 6, 5,
		6, 1, 2,
		
		4, 3, 0,
		3, 4, 7
	};
	
	context->skybox_mesh = MeshInit(&vertex_formats->v3_format,
									ArraySize(vertices), vertices,
									ArraySize(indices), indices);
	
	context->material_buffer = GPUBufferAlloc(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
											  VK_BUFFER_USAGE_TRANSFER_DST_BIT,
											  VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
											  sizeof(GPU_Material) * SCENE_MAX_GPU_MATERIALS);
	
	RenderContextLoadShaders(context, arena);
	RenderContextCreateUnitSphereMesh(context, arena);
	RenderContextCreatePerFrameObjects(context);
}

internal void
RenderContextDestroy(RenderContext *context)
{
	RenderContextDestroyPerFrameObjects(context);
	
	SamplerDestroy(&context->linear_sampler);
	
	MeshDestroy(&context->light_sphere_mesh);
	MeshDestroy(&context->skybox_mesh);
	
	GPUBufferDestroy(&context->material_buffer);
	GPUBufferDestroy(&context->cubemap_capture_transforms);
	
	ImageDestroy(&context->brdf_lut_image);
	ImageDestroy(&context->skybox_cubemap);
	ImageDestroy(&context->environment_probe.irradiance);
	ImageDestroy(&context->environment_probe.prefilter);
	
	ShaderProgramDestroy(&context->ambient_lighting_program);
	ShaderProgramDestroy(&context->direct_lighting_point_program);
	ShaderProgramDestroy(&context->model_program);
	ShaderProgramDestroy(&context->hdr_to_environment_cubemap_program);
	ShaderProgramDestroy(&context->irradiance_map_program);
	ShaderProgramDestroy(&context->prefilter_map_program);
	ShaderProgramDestroy(&context->skybox_program);
	ShaderProgramDestroy(&context->brdf_lut_program);
}

internal u32
RenderContextUploadMesh(RenderContext *context, Mesh *mesh)
{
	RenderMesh render_mesh = {0};
	{
		render_mesh.original = mesh;
		
		render_mesh.is_merged = false;
		
		render_mesh.first_vertex = 0;
		render_mesh.first_index = 0;
		render_mesh.index_count = mesh->index_count;
	}
	
	context->meshes[context->mesh_count] = render_mesh;
	
	return context->mesh_count++;
}

internal u32
RenderContextUploadMaterial(RenderContext *context, Assets *assets, Material *material)
{
	context->materials[context->material_count] = *material;
	
	GPU_Material gpu_material = {0};
	
	gpu_material.diffuse_texture            = FetchStandardImageView(AssetsImageFromHandle(assets, material->diffuse_texture_handle))->resource_id;
	gpu_material.normal_texture             = FetchStandardImageView(AssetsImageFromHandle(assets, material->normal_texture_handle))->resource_id;
	gpu_material.emissive_texture           = FetchStandardImageView(AssetsImageFromHandle(assets, material->emissive_texture_handle))->resource_id;
	gpu_material.metallic_roughness_texture = FetchStandardImageView(AssetsImageFromHandle(assets, material->metallic_roughness_texture_handle))->resource_id;
	gpu_material.ambient_texture            = FetchStandardImageView(AssetsImageFromHandle(assets, material->ambient_texture_handle))->resource_id;
	
	GPUBufferWrite(&context->material_buffer, &gpu_material,
				   sizeof(GPU_Material),
				   sizeof(GPU_Material) * context->material_count);
	
	return context->material_count++;
}

internal void
RenderPassGenerateBRDFLookUp(RenderContext *render_context, RenderInfo *render_info, void *context)
{
	CommandBuffer *cmd = &render_context->cmd;
	
	CmdBeginRendering(cmd, render_info);
	{
		GraphicsPipelineDef pipeline_def = GraphicsPipelineDefInitDefault(&render_context->brdf_lut_program, 0);
		pipeline_def.depth_stencil_state.depth_test_enabled = false;
		pipeline_def.depth_stencil_state.depth_write_enabled = false;
		pipeline_def.colour_attachment_count = 1;
		pipeline_def.colour_attachment_formats[0] = VK_FORMAT_R32G32_SFLOAT;
		
		PipelineState st = FetchGraphicsPipeline(&pipeline_def);
		
		CmdBindBindless(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, st.layout);
		CmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, st.pipeline);
		
		CmdDrawVerticesN(cmd, 3);
	}
	CmdEndRendering(cmd);
}

internal void
RenderContextGenerateBRDFLookUp(RenderContext *context, RenderGraph *graph)
{
	context->brdf_lut_image = ImageAlloc2D(512, 512, VK_FORMAT_R32G32_SFLOAT, 1);
	
	RenderPass render_pass = {0};
	render_pass.type = RenderPassType_Graphics;
	render_pass.graphics.Record = RenderPassGenerateBRDFLookUp;
	render_pass.graphics.attachment_count = 1;
	render_pass.graphics.attachments[0] = RenderingAttachmentInitColour(VK_ATTACHMENT_LOAD_OP_CLEAR,
																		FetchStandardImageView(&context->brdf_lut_image),
																		0, v4(0.f, 0.f, 0.f, 1.f));
	
	RenderGraphPush(graph, &render_pass);
}

internal void
RenderPassExportHDRCubemap(RenderContext *render_context, RenderInfo *render_info, void *context)
{
	CommandBuffer *cmd = &render_context->cmd;
	
	CmdBeginRendering(cmd, render_info);
	{
		struct
		{
			u64 transform_buffer;
			u32 hdr_image_id;
			u32 linear_sampler_id;
			u32 _padding[2];
		}
		args;
		
		Image *hdr_image = *((Image **)context);
		
		args.transform_buffer = render_context->cubemap_capture_transforms.device_address;
		args.hdr_image_id = FetchStandardImageView(hdr_image)->resource_id;
		args.linear_sampler_id = render_context->linear_sampler.resource_id;
		
		GraphicsPipelineDef pipeline_def = GraphicsPipelineDefInitDefault(&render_context->hdr_to_environment_cubemap_program, &vertex_formats->v3_format);
		pipeline_def.depth_stencil_state.depth_test_enabled = false;
		pipeline_def.depth_stencil_state.depth_write_enabled = false;
		pipeline_def.colour_attachment_count = 1;
		pipeline_def.colour_attachment_formats[0] = VK_FORMAT_R32G32B32A32_SFLOAT;
		pipeline_def.view_mask = 0b111111;
		
		PipelineState st = FetchGraphicsPipeline(&pipeline_def);
		
		CmdBindBindless(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, st.layout);
		CmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, st.pipeline);
		
		CmdPushConstants(cmd,
						 st.layout,
						 VK_SHADER_STAGE_ALL_GRAPHICS,
						 sizeof(args), &args, 0);
		
		MeshBindCmd(&render_context->skybox_mesh, cmd);
		MeshDrawCmd(&render_context->skybox_mesh, cmd);
	}
	CmdEndRendering(cmd);
	
	CmdPrepareForMipmapping(cmd, &render_context->skybox_cubemap);
	CmdGenerateMipmaps(cmd, &render_context->skybox_cubemap);
	
	DebugLog("Created Environment Cubemap.");
}

internal void
RenderPassGenerateIrradianceMap(RenderContext *render_context, RenderInfo *render_info, void *context)
{
	CommandBuffer *cmd = &render_context->cmd;
	
	CmdBeginRendering(cmd, render_info);
	{
		struct
		{
			u64 transform_buffer;
			u32 environment_map_id;
			u32 linear_sampler_id;
			u32 _padding[2];
		}
		args;
		
		args.transform_buffer = render_context->cubemap_capture_transforms.device_address;
		args.environment_map_id = FetchStandardImageView(&render_context->skybox_cubemap)->resource_id;
		args.linear_sampler_id = render_context->linear_sampler.resource_id;
		
		GraphicsPipelineDef pipeline_def = GraphicsPipelineDefInitDefault(&render_context->irradiance_map_program, &vertex_formats->v3_format);
		pipeline_def.depth_stencil_state.depth_test_enabled = false;
		pipeline_def.depth_stencil_state.depth_write_enabled = false;
		pipeline_def.colour_attachment_count = 1;
		pipeline_def.colour_attachment_formats[0] = VK_FORMAT_R32G32B32A32_SFLOAT;
		pipeline_def.view_mask = 0b111111;
		
		PipelineState st = FetchGraphicsPipeline(&pipeline_def);
		
		CmdBindBindless(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, st.layout);
		CmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, st.pipeline);
		
		CmdPushConstants(cmd,
						 st.layout,
						 VK_SHADER_STAGE_ALL_GRAPHICS,
						 sizeof(args), &args, 0);
		
		MeshBindCmd(&render_context->skybox_mesh, cmd);
		MeshDrawCmd(&render_context->skybox_mesh, cmd);
	}
	CmdEndRendering(cmd);
	
	CmdPrepareForMipmapping(cmd, &render_context->environment_probe.irradiance);
	CmdGenerateMipmaps(cmd, &render_context->environment_probe.irradiance);
	
	DebugLog("Created Irradiance Cubemap.");
}

internal void
RenderContextGenerateEnvironmentMap(RenderContext *context, RenderGraph *graph, Image *image)
{
	context->skybox_cubemap = ImageAllocCubemap(1024, VK_FORMAT_R32G32B32A32_SFLOAT, 4);
	
	RenderPass render_pass = {0};
	render_pass.type = RenderPassType_Graphics;
	render_pass.graphics.Record = RenderPassExportHDRCubemap;
	render_pass.graphics.view_mask = 0b111111;
	render_pass.graphics.view_count = 1;
	render_pass.graphics.views[0] = FetchStandardImageView(image);
	render_pass.graphics.attachment_count = 1;
	render_pass.graphics.attachments[0] = RenderingAttachmentInitColour(VK_ATTACHMENT_LOAD_OP_CLEAR,
																		FetchStandardImageView(&context->skybox_cubemap),
																		0, v4(0.f, 0.f, 0.f, 1.f));
	
	MemoryCopy(render_pass.context, &image, sizeof(Image *));
	
	RenderGraphPush(graph, &render_pass);
}

internal void
RenderPassGeneratePrefilterMap(RenderContext *render_context, RenderInfo *render_info, void *context)
{
	CommandBuffer *cmd = &render_context->cmd;
	
	CmdBeginRendering(cmd, render_info);
	{
		struct
		{
			u64 transform_buffer;
			f32 roughness;
			u32 environment_map_id;
			u32 linear_sampler_id;
			u32 _padding;
		}
		args;
		
		args.transform_buffer = render_context->cubemap_capture_transforms.device_address;
		args.roughness = *((f32 *)context);
		args.environment_map_id = FetchStandardImageView(&render_context->skybox_cubemap)->resource_id;
		args.linear_sampler_id = render_context->linear_sampler.resource_id;
		
		GraphicsPipelineDef pipeline_def = GraphicsPipelineDefInitDefault(&render_context->prefilter_map_program, &vertex_formats->v3_format);
		pipeline_def.depth_stencil_state.depth_test_enabled = false;
		pipeline_def.depth_stencil_state.depth_write_enabled = false;
		pipeline_def.colour_attachment_count = 1;
		pipeline_def.colour_attachment_formats[0] = VK_FORMAT_R32G32B32A32_SFLOAT;
		pipeline_def.view_mask = 0b111111;
		
		PipelineState st = FetchGraphicsPipeline(&pipeline_def);
		
		CmdBindBindless(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, st.layout);
		CmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, st.pipeline);
		
		CmdPushConstants(cmd,
						 st.layout,
						 VK_SHADER_STAGE_ALL_GRAPHICS,
						 sizeof(args), &args, 0);
		
		MeshBindCmd(&render_context->skybox_mesh, cmd);
		MeshDrawCmd(&render_context->skybox_mesh, cmd);
	}
	CmdEndRendering(cmd);
	
	DebugLog("Created a Prefilter Cubemap mip level.");
}

internal void
RenderContextGenerateEnvironmentProbeFromEnvironmentCubemap(RenderContext *context, RenderGraph *graph)
{
	// NOTE(kp): Irradiance Map.
	{
		context->environment_probe.irradiance = ImageAllocCubemap(32, VK_FORMAT_R32G32B32A32_SFLOAT, 4);
		
		RenderPass render_pass = {0};
		render_pass.type = RenderPassType_Graphics;
		render_pass.graphics.Record = RenderPassGenerateIrradianceMap;
		render_pass.graphics.view_mask = 0b111111;
		render_pass.graphics.view_count = 1;
		render_pass.graphics.views[0] = FetchStandardImageView(&context->skybox_cubemap);
		render_pass.graphics.attachment_count = 1;
		render_pass.graphics.attachments[0] = RenderingAttachmentInitColour(VK_ATTACHMENT_LOAD_OP_CLEAR,
																			FetchStandardImageView(&context->environment_probe.irradiance),
																			0, v4(0.f, 0.f, 0.f, 1.f));
		
		RenderGraphPush(graph, &render_pass);
	}
	
	// NOTE(kp): Prefilter Map.
	{
		context->environment_probe.prefilter = ImageAllocCubemap(128, VK_FORMAT_R32G32B32A32_SFLOAT, 4);
		
		i32 mipmap_count = context->environment_probe.prefilter.mipmap_count;
		
		for(i32 mip_level = 0; mip_level < mipmap_count; mip_level++)
		{
			ImageView *prefilter_view = FetchImageView(&context->environment_probe.prefilter,
													   ImageLayerCount(&context->environment_probe.prefilter),
													   0, mip_level);
			
			f32 roughness = (f32)(mip_level) / (f32)(mipmap_count - 1);
			
			RenderPass render_pass = {0};
			render_pass.type = RenderPassType_Graphics;
			MemoryCopy(render_pass.context, &roughness, sizeof(f32));
			render_pass.graphics.Record = RenderPassGeneratePrefilterMap;
			render_pass.graphics.view_mask = 0b111111;
			render_pass.graphics.view_count = 1;
			render_pass.graphics.views[0] = FetchStandardImageView(&context->skybox_cubemap);
			render_pass.graphics.attachment_count = 1;
			render_pass.graphics.attachments[0] = RenderingAttachmentInitColour(VK_ATTACHMENT_LOAD_OP_CLEAR,
																				prefilter_view,
																				0, v4(0.f, 0.f, 0.f, 1.f));
			
			RenderGraphPush(graph, &render_pass);
		}
	}
}

// TODO(kp): Skybox and environment probes should be local to a scene, not just part of the render context?
internal void
RenderContextSetSkybox(RenderContext *context, RenderGraph *out, Image *image)
{
	RenderContextGenerateEnvironmentMap(context, out, image);
	RenderContextGenerateEnvironmentProbeFromEnvironmentCubemap(context, out);
}
