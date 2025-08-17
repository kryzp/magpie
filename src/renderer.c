
internal Mesh
MeshInit(VertexFormat *format,
		 u32 vertex_count, void *vertices,
		 u32 index_count, u16 *indices)
{
	Mesh mesh = {0};
	mesh.vertex_format = format;
	mesh.vertex_count = vertex_count;
	mesh.index_count = index_count;
	
	u64 vertex_buffer_size = vertex_count * format->vertex_size;
	u64 index_buffer_size = index_count * sizeof(u16);
	
	mesh.vertex_buffer = GPUBufferAllocate(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
										   VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
										   vertex_buffer_size);
	
	mesh.index_buffer = GPUBufferAllocate(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
										  VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
										  index_buffer_size);
	
	GPUBuffer staging_buffer = GPUBufferAllocate(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
												 VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
												 vertex_buffer_size + index_buffer_size);
	{
		GPUBufferWrite(&staging_buffer, vertices, vertex_buffer_size, 0);
		GPUBufferWrite(&staging_buffer, indices, index_buffer_size, vertex_buffer_size);
		
		CommandBuffer cmd = BeginGraphicsInstantSubmit();
		{
			VkBufferCopy stage_to_vertex_copy = {0};
			stage_to_vertex_copy.srcOffset = 0;
			stage_to_vertex_copy.dstOffset = 0;
			stage_to_vertex_copy.size = vertex_buffer_size;
			
			CmdCopyBufferToBuffer(&cmd,
								  &staging_buffer,
								  &mesh.vertex_buffer,
								  1, &stage_to_vertex_copy);
			
			VkBufferCopy stage_to_index_copy = {0};
			stage_to_index_copy.srcOffset = vertex_buffer_size;
			stage_to_index_copy.dstOffset = 0;
			stage_to_index_copy.size = index_buffer_size;
			
			CmdCopyBufferToBuffer(&cmd,
								  &staging_buffer,
								  &mesh.index_buffer,
								  1, &stage_to_index_copy);
		}
		EndGraphicsInstantSubmit(&cmd);
	}
	GraphicsWaitIdle();
	GPUBufferDestroy(&staging_buffer);
	
	return mesh;
}

internal void
MeshDestroy(Mesh *mesh)
{
	GPUBufferDestroy(&mesh->vertex_buffer);
	GPUBufferDestroy(&mesh->index_buffer);
}

internal RenderAttachment
RenderAttachmentInitColour(VkAttachmentLoadOp load_op,
						   ImageView *view,
						   ImageView *resolve,
						   v4 clear_colour)
{
	RenderAttachment attachment = {0};
	attachment.view = view;
	attachment.resolve = resolve;
	attachment.resolve_mode = resolve ? VK_RESOLVE_MODE_AVERAGE_BIT : VK_RESOLVE_MODE_NONE;
	attachment.load_op = load_op;
	attachment.store_op = VK_ATTACHMENT_STORE_OP_STORE;
	attachment.clear_colour = clear_colour;
	
	return attachment;
}

internal RenderAttachment
RenderAttachmentInitDepth(VkAttachmentLoadOp load_op,
						  ImageView *view,
						  ImageView *resolve,
						  f32 clear_depth,
						  u32 clear_stencil)
{
	RenderAttachment attachment = {0};
	attachment.view = view;
	attachment.resolve = resolve;
	attachment.resolve_mode = resolve ? VK_RESOLVE_MODE_SAMPLE_ZERO_BIT : VK_RESOLVE_MODE_NONE;
	attachment.load_op = load_op;
	attachment.store_op = VK_ATTACHMENT_STORE_OP_STORE;
	attachment.clear_depth = clear_depth;
	attachment.clear_stencil = clear_stencil;
	
	return attachment;
}

internal void
RendererPushRenderPass(Renderer *renderer, RenderPassDef *pass)
{
	renderer->passes[renderer->pass_count].type = RenderPassType_Render;
	renderer->passes[renderer->pass_count].index = renderer->internal_render_pass_count;
	
	renderer->internal_render_passes[renderer->internal_render_pass_count] = *pass;
	renderer->internal_render_pass_count++;
	
	renderer->pass_count++;
}

internal void
RendererPushComputePass(Renderer *renderer, ComputePassDef *pass)
{
	renderer->passes[renderer->pass_count].type = RenderPassType_Compute;
	renderer->passes[renderer->pass_count].index = renderer->internal_compute_pass_count;
	
	renderer->internal_compute_passes[renderer->internal_compute_pass_count] = *pass;
	renderer->internal_compute_pass_count++;
	
	renderer->pass_count++;
}

internal void
RendererExecuteRenderPasses(Renderer *renderer, CommandBuffer *cmd)
{
	for(i32 i = 0; i < renderer->pass_count; i++)
	{
		RenderPassType pass_type = renderer->passes[i].type;
		u32 pass_index = renderer->passes[i].index;
		
		switch(pass_type)
		{
			case RenderPassType_Render:
			{
				RenderPassDef *render_pass = renderer->internal_render_passes + pass_index;
				
				RenderInfo render_info = {0};
				
				for(i32 j = 0; j < render_pass->attachment_count; j++)
				{
					RenderAttachment *attachment = render_pass->attachments + j;
					Image *attachment_image = attachment->view->image;
					
					render_info.width = attachment_image->width;
					render_info.height = attachment_image->height;
					render_info.samples = attachment_image->samples;
					
					render_info.view_mask = render_pass->view_mask;
					
					if(ImageIsDepth(attachment_image))
					{
						RenderInfoAddDepthAttachment(&render_info,
													 attachment->load_op,
													 attachment->view,
													 attachment->resolve,
													 attachment->clear_depth,
													 attachment->clear_stencil);
						
						CmdTransitionImageLayout(cmd,
												 attachment_image,
												 VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
					}
					else
					{
						RenderInfoAddColourAttachment(&render_info,
													  attachment->load_op,
													  attachment->view,
													  attachment->resolve,
													  attachment->clear_colour);
						
						CmdTransitionImageLayout(cmd,
												 attachment_image,
												 VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
					}
				}
				
				for(i32 j = 0; j < render_pass->view_count; j++)
				{
					ImageView *view = render_pass->views + j;
					
					if(ImageIsDepth(view->image))
					{
						CmdTransitionImageLayout(cmd,
												 view->image,
												 VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);
					}
					else
					{
						CmdTransitionImageLayout(cmd,
												 view->image,
												 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
					}
				}
				
				render_pass->Record(renderer, cmd, &render_info);
			}
			break;
			
			case RenderPassType_Compute:
			{
				//ComputePassDef *compute_pass = renderer->internal_compute_passes + pass_index;
				
				// TODO(kp)
			}
			break;
		}
	}
}

#define ENVIRONMENT_MAP_RESOLUTION 1024

internal void
RendererRenderPassExportHDRCubemap(Renderer *renderer, CommandBuffer *cmd, RenderInfo *render_info)
{
	BindlessBindCombinedImage(&graphics_device->bindless, 0,
							  &renderer->environment_map_hdr_image_view,
							  &renderer->environment_map_sampler);
	
	m4 capture_projection_matrix = M4Perspective(90.0f, 1.0f, 0.1f, 10.0f);
	
	m4 capture_view_matrices[] = {
		M4LookAt(v3(0.f, 0.f, 0.f), v3( 1.f, 0.f, 0.f), v3(0.f, 1.f, 0.f)), // X+
		M4LookAt(v3(0.f, 0.f, 0.f), v3(-1.f, 0.f, 0.f), v3(0.f, 1.f, 0.f)), // X-
		M4LookAt(v3(0.f, 0.f, 0.f), v3( 0.f, 1.f, 0.f), v3(0.f, 0.f, 1.f)), // Y+
		M4LookAt(v3(0.f, 0.f, 0.f), v3( 0.f,-1.f, 0.f), v3(0.f, 0.f,-1.f)), // Y-
		M4LookAt(v3(0.f, 0.f, 0.f), v3( 0.f, 0.f,-1.f), v3(0.f, 1.f, 0.f)), // Z+
		M4LookAt(v3(0.f, 0.f, 0.f), v3( 0.f, 0.f, 1.f), v3(0.f, 1.f, 0.f)), // Z-
	};
	
	CmdBeginRendering(cmd, render_info);
	{
		GPUBuffer transform_buffer = GPUBufferAllocate(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
													   VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
													   sizeof(m4) * 6);
		
		for(i32 i = 0; i < 6; i++)
		{
			m4 m = M4MultiplyM4(capture_projection_matrix, capture_view_matrices[i]);
			GPUBufferWrite(&transform_buffer, &m, sizeof(m4), sizeof(m4) * i);
		}
		
		CmdBindDescriptors(cmd,
						   VK_PIPELINE_BIND_POINT_GRAPHICS,
						   renderer->environment_map_pipeline_layout,
						   0,
						   1, &graphics_device->bindless.bindless_set,
						   0, 0);
		
		CmdBindPipeline(cmd,
						VK_PIPELINE_BIND_POINT_GRAPHICS,
						renderer->environment_map_pipeline);
		
		VkViewport viewport = { 0, 0, ENVIRONMENT_MAP_RESOLUTION, ENVIRONMENT_MAP_RESOLUTION };
		CmdSetViewport(cmd, viewport);
		
		CmdBindVertexBuffer(cmd, 0, &renderer->environment_cube_mesh.vertex_buffer, 0);
		CmdBindIndexBuffer(cmd, &renderer->environment_cube_mesh.index_buffer, 0);
		
		CmdPushConstants(cmd,
						 renderer->environment_map_pipeline_layout,
						 VK_SHADER_STAGE_ALL_GRAPHICS,
						 0,
						 sizeof(u64), &transform_buffer.device_address);
		
		CmdDrawIndexed(cmd, renderer->environment_cube_mesh.index_count, 1, 0, 0, 0);
		
		GPUBufferDestroy(&transform_buffer);
	}
	CmdEndRendering(cmd);
	
	CmdTransitionImageLayout(cmd, &renderer->environment_map_cubemap, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	
	CmdGenerateMipmaps(cmd, &renderer->environment_map_cubemap);
}

internal void
RendererGenerateEnvironmentMap(Renderer *renderer, MemoryArena *arena)
{
	typedef struct EnvironmentMapVertex
	{
		v3 position;
	}
	EnvironmentMapVertex;
	
	AddVertexBinding(&renderer->environment_cube_vertex_format, sizeof(EnvironmentMapVertex), VK_VERTEX_INPUT_RATE_VERTEX);
	AddVertexAttribute(&renderer->environment_cube_vertex_format, VK_FORMAT_R32G32B32_SFLOAT, offsetof(EnvironmentMapVertex, position));
	
	EnvironmentMapVertex vertices[] = {
		{ { -1.0f,  1.0f, -1.0f } },
		{ { -1.0f, -1.0f, -1.0f } },
		{ {  1.0f, -1.0f, -1.0f } },
		{ {  1.0f,  1.0f, -1.0f } },
		{ { -1.0f,  1.0f,  1.0f } },
		{ { -1.0f, -1.0f,  1.0f } },
		{ {  1.0f, -1.0f,  1.0f } },
		{ {  1.0f,  1.0f,  1.0f } }
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
	
	renderer->environment_cube_mesh = MeshInit(&renderer->environment_cube_vertex_format,
											   ArraySize(vertices), vertices,
											   ArraySize(indices), indices);
	
	renderer->environment_map_program.push_constant_size = sizeof(u64);
	renderer->environment_map_program.layout_count = 1;
	renderer->environment_map_program.layouts[0] = graphics_device->bindless.bindless_layout;
	
	renderer->environment_map_program.stage_count = 2;
	renderer->environment_map_program.stages[0] = ShaderStageLoadFromBytecode(arena, str8("res/vertex.spv"), VK_SHADER_STAGE_VERTEX_BIT);
	renderer->environment_map_program.stages[1] = ShaderStageLoadFromBytecode(arena, str8("res/fragment.spv"), VK_SHADER_STAGE_FRAGMENT_BIT);
	
	renderer->environment_map_hdr_image = ImageFromPath(str8("res/environment_map.hdr"));
	
	renderer->environment_map_cubemap = ImageAllocate(ENVIRONMENT_MAP_RESOLUTION, ENVIRONMENT_MAP_RESOLUTION, 1,
													  VK_FORMAT_R32G32B32A32_SFLOAT,
													  VK_IMAGE_VIEW_TYPE_CUBE,
													  VK_IMAGE_TILING_OPTIMAL,
													  4,
													  VK_SAMPLE_COUNT_1_BIT,
													  0,
													  0);
	
	renderer->environment_map_hdr_image_view = GetStandardImageView(&renderer->environment_map_hdr_image);
	renderer->environment_map_cubemap_view = GetStandardImageView(&renderer->environment_map_cubemap);
	
	renderer->environment_map_sampler = SamplerInitFilter(VK_FILTER_NEAREST);
	
	GraphicsPipelineDef env_map_pipeline_def = {0};
	env_map_pipeline_def.program = &renderer->environment_map_program;
	env_map_pipeline_def.vertex_format = &renderer->environment_cube_vertex_format;
	env_map_pipeline_def.cull_mode = VK_CULL_MODE_BACK_BIT;
	env_map_pipeline_def.front_face = VK_FRONT_FACE_CLOCKWISE;
	env_map_pipeline_def.blend_state = BlendStateDefault();
	env_map_pipeline_def.depth_stencil_state = DepthStencilStateDefault();
	env_map_pipeline_def.depth_stencil_state.depth_test_enabled = 0;
	env_map_pipeline_def.depth_stencil_state.depth_write_enabled = 0;
	env_map_pipeline_def.colour_attachment_count = 1;
	env_map_pipeline_def.colour_attachment_formats[0] = VK_FORMAT_R32G32B32A32_SFLOAT;
	env_map_pipeline_def.has_depth_attachment = 0;
	env_map_pipeline_def.min_sample_shading_enabled = 1;
	env_map_pipeline_def.min_sample_shading = 0.2f;
	env_map_pipeline_def.samples = VK_SAMPLE_COUNT_1_BIT;
	env_map_pipeline_def.view_mask = 0b111111;
	
	renderer->environment_map_pipeline_layout = PipelineLayoutCreate(&renderer->environment_map_program);
	
	renderer->environment_map_pipeline = GraphicsPipelineCreate(renderer->environment_map_pipeline_layout,
																&env_map_pipeline_def);
	
	RenderPassDef render_pass = {0};
	render_pass.view_count = 1;
	render_pass.views[0] = renderer->environment_map_hdr_image_view;
	render_pass.attachment_count = 1;
	render_pass.attachments[0] = RenderAttachmentInitColour(VK_ATTACHMENT_LOAD_OP_LOAD,
															&renderer->environment_map_cubemap_view,
															0,
															v4(0.f, 0.f, 0.f, 1.f));
	render_pass.view_mask = 0b111111;
	render_pass.Record = RendererRenderPassExportHDRCubemap;
	
	RendererPushRenderPass(renderer, &render_pass);
}

internal void
RendererInit(Renderer *renderer, MemoryArena *arena) // TODO(kp): Arena is temporarily passed in here to do loading in of shaders.
{
	RendererGenerateEnvironmentMap(renderer, arena);
}

internal void
RendererDestroy(Renderer *renderer)
{
	MeshDestroy(&renderer->environment_cube_mesh);
	ShaderStageDestroy(&renderer->environment_map_program.stages[0]);
	ShaderStageDestroy(&renderer->environment_map_program.stages[1]);
	ImageDestroy(&renderer->environment_map_hdr_image);
	ImageDestroy(&renderer->environment_map_cubemap);
	ImageViewDestroy(&renderer->environment_map_hdr_image_view);
	ImageViewDestroy(&renderer->environment_map_cubemap_view);
	SamplerDestroy(&renderer->environment_map_sampler);
	PipelineLayoutDestroy(renderer->environment_map_pipeline_layout);
	PipelineDestroy(renderer->environment_map_pipeline);
}

internal void
RendererBeginFrame(Renderer *renderer)
{
	renderer->present_cmd = BeginGraphicsPresent();
}

internal void
RendererEndFrame(Renderer *renderer)
{
	// NOTE(kp): Perform render passes.
	RendererExecuteRenderPasses(renderer, &renderer->present_cmd);
	
	EndGraphicsPresent(&renderer->present_cmd);
	
	renderer->pass_count = 0;
	renderer->internal_render_pass_count = 0;
	renderer->internal_compute_pass_count = 0;
}
