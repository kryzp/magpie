
// TODO(kp): (Not in order of importance)
//           [x]  1. Pipeline state caching, two seperate hash
//                   tables for layouts and pipelines. Also cache
//                   image views automaticlly.
//           [x]  2. Irradiance + Prefilter map generation.
//           [x]  3. Remove Combined Image-Sampler from bindless
//                   and add seperate image + sampler lists.
//           [ ]  4. Proper asset system for shader, texture and model loading.
//           [ ]  5. Bindless material system from C++ pre-rework
//                   (with bindless, a material is *just* data you pass to a
//                   shader, since textures are just parameters
//                   like any other into the material).
//           [ ]  6. Debug renderer (lines, spheres, etc...) (seperate thing)
//           [ ]  7. Text rendering (fonts)
//           [ ]  8. Scene system (e.g: a scene might have some
//                   objects to render, multiple lighting probes, etc...)
//           [ ]  9. More Assert(...)'s in the codebase.
//           [ ] 10. Well commented codebase.
//           [ ] 11. Due to dynamic rendering, there is a lot of data you have to duplicate
//                   between graphics pipelines and render info's (view mask, formats, etc...),
//                   figure out a way to merge this together. Maybe pass render info
//                   into GraphicsPipelineCreate(...)?
//           [ ] 12. (This applies to all bindless resources.)
//                   resource_id should *not* be assigned in the
//                   graphics device. In fact, the graphics device
//                   should not be managing bindless in the first
//                   place, that should be a policy of the renderer.
//                   --> Maybe in the future, have a BindlessResources
//                       struct in the high level, that different renderers
//                       can use to manage their bindless resources
//           [ ] 13. Mipmap generation should happen automatically when executing render passes.
//                   --> When that is achieved, automatically call CmdBeginRendering(...) and
//                       CmdEndRendering(...) around the Record(...) function, since mipmaps
//                       are pretty much the only reason I don't already do that.
//           [ ] 14. Generic hash table implementation.
//           [ ] 15. Deferred Rendering.
//           [ ] 16. Split up files to make the code easier to manage.
//           [ ] 17. Investigate how I'm taking up ~100kb of memory in the allocated 32MB?

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

internal void
CmdBindMesh(CommandBuffer *cmd, Mesh *mesh)
{
	CmdBindVertexBuffer(cmd, 0, &mesh->vertex_buffer, 0);
	CmdBindIndexBuffer(cmd, &mesh->index_buffer, 0);
}

internal RenderingAttachment
RenderingAttachmentInitColour(VkAttachmentLoadOp load_op,
							  ImageView *view,
							  ImageView *resolve,
							  v4 clear_colour)
{
	RenderingAttachment attachment = {0};
	
	attachment.info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	attachment.info.imageView = view->view;
	attachment.info.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	attachment.info.loadOp = load_op;
	attachment.info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachment.info.clearValue = (VkClearValue){ .color = { clear_colour.r, clear_colour.g, clear_colour.b, clear_colour.a } };
	
	if(resolve)
	{
		attachment.info.resolveImageView = resolve->view;
		attachment.info.resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		attachment.info.resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT;
	}
	else
	{
		attachment.info.resolveImageView = VK_NULL_HANDLE;
		attachment.info.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachment.info.resolveMode = VK_RESOLVE_MODE_NONE;
	}
	
	attachment.image = view->image;
	
	attachment.width  = view->image->width  >> view->base_mip_level;
	attachment.height = view->image->height >> view->base_mip_level;
	
	return attachment;
}

internal RenderingAttachment
RenderingAttachmentInitDepth(VkAttachmentLoadOp load_op,
							 ImageView *view,
							 ImageView *resolve,
							 f32 clear_depth,
							 u32 clear_stencil)
{
	RenderingAttachment attachment = {0};
	
	attachment.info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	attachment.info.imageView = view->view;
	attachment.info.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	attachment.info.loadOp = load_op;
	attachment.info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachment.info.clearValue = (VkClearValue){ .depthStencil = { clear_depth, clear_stencil } };
	
	if (resolve)
	{
		attachment.info.resolveImageView = resolve->view;
		attachment.info.resolveImageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		attachment.info.resolveMode = VK_RESOLVE_MODE_SAMPLE_ZERO_BIT;
	}
	else
	{
		attachment.info.resolveImageView = VK_NULL_HANDLE;
		attachment.info.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachment.info.resolveMode = VK_RESOLVE_MODE_NONE;
	}
	
	attachment.image = view->image;
	
	attachment.width  = view->image->width  >> view->base_mip_level;
	attachment.height = view->image->height >> view->base_mip_level;
	
	return attachment;
}

internal void
RendererPushRenderPass(Renderer *renderer, RenderPass *pass)
{
	renderer->passes[renderer->pass_count] = *pass;
	renderer->pass_count++;
}

internal void
RendererExecuteRenderPasses(Renderer *renderer, CommandBuffer *cmd)
{
	for(i32 i = 0; i < renderer->pass_count; i++)
	{
		RenderPass pass = renderer->passes[i];
		
		switch(pass.type)
		{
			case RenderPassType_Graphics:
			{
				RenderInfo render_info = {0};
				render_info.view_mask = pass.graphics.view_mask;
				
				for(i32 j = 0; j < pass.graphics.attachment_count; j++)
				{
					RenderingAttachment *attachment = pass.graphics.attachments + j;
					
					render_info.width = attachment->width;
					render_info.height = attachment->height;
					
					render_info.samples = attachment->image->samples;
					
					if(ImageIsDepth(attachment->image))
					{
						render_info.depth_attachment = attachment->info;
						
						CmdTransitionImageLayout(cmd, attachment->image,
												 VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
					}
					else
					{
						render_info.colour_attachments[render_info.colour_attachment_count++] = attachment->info;
						
						CmdTransitionImageLayout(cmd, attachment->image,
												 VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
					}
				}
				
				for(i32 j = 0; j < pass.graphics.view_count; j++)
				{
					ImageView *view = pass.graphics.views[j];
					
					if(ImageIsDepth(view->image))
					{
						CmdTransitionImageLayout(cmd, view->image,
												 VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);
					}
					else
					{
						CmdTransitionImageLayout(cmd, view->image,
												 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
					}
				}
				
				pass.graphics.Record(renderer, cmd, &render_info, pass.context);
			}
			break;
			
			case RenderPassType_Compute:
			{
				// TODO(kp)
			}
			break;
		}
	}
	
	renderer->pass_count = 0;
}

internal void
RendererRenderPassExportHDRCubemap(Renderer *renderer, CommandBuffer *cmd, RenderInfo *render_info, void *context)
{
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
		
		args.transform_buffer = renderer->cubemap_capture_transforms.device_address;
		args.hdr_image_id = FetchStandardImageView(&renderer->environment_hdr_image)->resource_id;
		args.linear_sampler_id = renderer->linear_sampler.resource_id;
		
		GraphicsPipelineDef pipeline_def = GraphicsPipelineDefInitDefault(&renderer->environment_hdr_to_cubemap_program, &renderer->environment_cube_vertex_format);
		pipeline_def.depth_stencil_state.depth_test_enabled = false;
		pipeline_def.depth_stencil_state.depth_write_enabled = false;
		pipeline_def.colour_attachment_count = 1;
		pipeline_def.colour_attachment_formats[0] = VK_FORMAT_R32G32B32A32_SFLOAT;
		pipeline_def.view_mask = 0b111111;
		
		PipelineState st = FetchGraphicsPipeline(&pipeline_def);
		
		CmdBindBindless(cmd,
						VK_PIPELINE_BIND_POINT_GRAPHICS,
						st.layout);
		
		CmdBindPipeline(cmd,
						VK_PIPELINE_BIND_POINT_GRAPHICS,
						st.pipeline);
		
		CmdPushConstants(cmd,
						 st.layout,
						 VK_SHADER_STAGE_ALL_GRAPHICS,
						 sizeof(args), &args, 0);
		
		CmdBindMesh(cmd, &renderer->environment_cube_mesh);
		
		CmdDrawIndexed(cmd, renderer->environment_cube_mesh.index_count, 1, 0, 0, 0);
	}
	CmdEndRendering(cmd);
	
	CmdPrepareForMipmapping(cmd, &renderer->environment_cubemap);
	CmdGenerateMipmaps(cmd, &renderer->environment_cubemap);
}

internal void
RendererGenerateEnvironmentMap(Renderer *renderer)
{
	renderer->environment_cubemap = ImageAllocate(1024, 1024, 1,
												  VK_FORMAT_R32G32B32A32_SFLOAT,
												  VK_IMAGE_VIEW_TYPE_CUBE,
												  VK_IMAGE_TILING_OPTIMAL,
												  4,
												  VK_SAMPLE_COUNT_1_BIT,
												  false, false);
	
	RenderPass render_pass = {0};
	render_pass.type = RenderPassType_Graphics;
	render_pass.graphics.Record = RendererRenderPassExportHDRCubemap;
	render_pass.graphics.view_mask = 0b111111;
	render_pass.graphics.view_count = 1;
	render_pass.graphics.views[0] = FetchStandardImageView(&renderer->environment_hdr_image);
	render_pass.graphics.attachment_count = 1;
	render_pass.graphics.attachments[0] = RenderingAttachmentInitColour(VK_ATTACHMENT_LOAD_OP_LOAD,
																		FetchStandardImageView(&renderer->environment_cubemap),
																		0, v4(0.f, 0.f, 0.f, 1.f));
	
	RendererPushRenderPass(renderer, &render_pass);
}

internal void
RendererRenderPassGenerateIrradianceMap(Renderer *renderer, CommandBuffer *cmd, RenderInfo *render_info, void *context)
{
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
		
		args.transform_buffer = renderer->cubemap_capture_transforms.device_address;
		args.environment_map_id = FetchStandardImageView(&renderer->environment_cubemap)->resource_id;
		args.linear_sampler_id = renderer->linear_sampler.resource_id;
		
		GraphicsPipelineDef pipeline_def = GraphicsPipelineDefInitDefault(&renderer->irradiance_map_program, &renderer->environment_cube_vertex_format);
		pipeline_def.depth_stencil_state.depth_test_enabled = false;
		pipeline_def.depth_stencil_state.depth_write_enabled = false;
		pipeline_def.colour_attachment_count = 1;
		pipeline_def.colour_attachment_formats[0] = VK_FORMAT_R32G32B32A32_SFLOAT;
		pipeline_def.view_mask = 0b111111;
		
		PipelineState st = FetchGraphicsPipeline(&pipeline_def);
		
		CmdBindBindless(cmd,
						VK_PIPELINE_BIND_POINT_GRAPHICS,
						st.layout);
		
		CmdBindPipeline(cmd,
						VK_PIPELINE_BIND_POINT_GRAPHICS,
						st.pipeline);
		
		CmdPushConstants(cmd,
						 st.layout,
						 VK_SHADER_STAGE_ALL_GRAPHICS,
						 sizeof(args), &args, 0);
		
		CmdBindMesh(cmd, &renderer->environment_cube_mesh);
		
		CmdDrawIndexed(cmd, renderer->environment_cube_mesh.index_count, 1, 0, 0, 0);
	}
	CmdEndRendering(cmd);
	
	CmdPrepareForMipmapping(cmd, &renderer->environment_probe.irradiance);
	CmdGenerateMipmaps(cmd, &renderer->environment_probe.irradiance);
}

internal void
RendererRenderPassGeneratePrefilterMap(Renderer *renderer, CommandBuffer *cmd, RenderInfo *render_info, void *context)
{
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
		
		args.transform_buffer = renderer->cubemap_capture_transforms.device_address;
		args.roughness = *((f32 *)context);
		args.environment_map_id = FetchStandardImageView(&renderer->environment_cubemap)->resource_id;
		args.linear_sampler_id = renderer->linear_sampler.resource_id;
		
		GraphicsPipelineDef pipeline_def = GraphicsPipelineDefInitDefault(&renderer->prefilter_map_program, &renderer->environment_cube_vertex_format);
		pipeline_def.depth_stencil_state.depth_test_enabled = false;
		pipeline_def.depth_stencil_state.depth_write_enabled = false;
		pipeline_def.colour_attachment_count = 1;
		pipeline_def.colour_attachment_formats[0] = VK_FORMAT_R32G32B32A32_SFLOAT;
		pipeline_def.view_mask = 0b111111;
		
		PipelineState st = FetchGraphicsPipeline(&pipeline_def);
		
		CmdBindBindless(cmd,
						VK_PIPELINE_BIND_POINT_GRAPHICS,
						st.layout);
		
		CmdBindPipeline(cmd,
						VK_PIPELINE_BIND_POINT_GRAPHICS,
						st.pipeline);
		
		CmdPushConstants(cmd,
						 st.layout,
						 VK_SHADER_STAGE_ALL_GRAPHICS,
						 sizeof(args), &args, 0);
		
		CmdBindMesh(cmd, &renderer->environment_cube_mesh);
		
		CmdDrawIndexed(cmd, renderer->environment_cube_mesh.index_count, 1, 0, 0, 0);
	}
	CmdEndRendering(cmd);
}

internal void
RendererGenerateEnvironmentProbeFromEnvironmentCubemap(Renderer *renderer, Image *environment_cubemap)
{
	// NOTE(kp): Irradiance Map.
	{
		renderer->environment_probe.irradiance = ImageAllocate(32, 32, 1,
															   VK_FORMAT_R32G32B32A32_SFLOAT,
															   VK_IMAGE_VIEW_TYPE_CUBE,
															   VK_IMAGE_TILING_OPTIMAL,
															   4,
															   VK_SAMPLE_COUNT_1_BIT,
															   false, false);
		
		RenderPass render_pass = {0};
		render_pass.type = RenderPassType_Graphics;
		render_pass.graphics.Record = RendererRenderPassGenerateIrradianceMap;
		render_pass.graphics.view_mask = 0b111111;
		render_pass.graphics.view_count = 1;
		render_pass.graphics.views[0] = FetchStandardImageView(&renderer->environment_cubemap);
		render_pass.graphics.attachment_count = 1;
		render_pass.graphics.attachments[0] = RenderingAttachmentInitColour(VK_ATTACHMENT_LOAD_OP_LOAD,
																			FetchStandardImageView(&renderer->environment_probe.irradiance),
																			0, v4(0.f, 0.f, 0.f, 1.f));
		
		RendererPushRenderPass(renderer, &render_pass);
	}
	
	// NOTE(kp): Prefilter Map.
	{
		renderer->environment_probe.prefilter = ImageAllocate(128, 128, 1,
															  VK_FORMAT_R32G32B32A32_SFLOAT,
															  VK_IMAGE_VIEW_TYPE_CUBE,
															  VK_IMAGE_TILING_OPTIMAL,
															  4,
															  VK_SAMPLE_COUNT_1_BIT,
															  false, false);
		
		i32 mipmap_count = renderer->environment_probe.prefilter.mipmap_count;
		
		for(i32 mip_level = 0; mip_level < mipmap_count; mip_level++)
		{
			ImageView *prefilter_view = FetchImageView(&renderer->environment_probe.prefilter,
													   GetImageLayerCount(&renderer->environment_probe.prefilter),
													   0, mip_level);
			
			f32 roughness = (f32)(mip_level) / (f32)(mipmap_count - 1);
			
			RenderPass render_pass = {0};
			render_pass.type = RenderPassType_Graphics;
			MemoryCopy(render_pass.context, &roughness, sizeof(f32));
			render_pass.graphics.Record = RendererRenderPassGeneratePrefilterMap;
			render_pass.graphics.view_mask = 0b111111;
			render_pass.graphics.view_count = 1;
			render_pass.graphics.views[0] = FetchStandardImageView(&renderer->environment_cubemap);
			render_pass.graphics.attachment_count = 1;
			render_pass.graphics.attachments[0] = RenderingAttachmentInitColour(VK_ATTACHMENT_LOAD_OP_LOAD,
																				prefilter_view,
																				0, v4(0.f, 0.f, 0.f, 1.f));
			
			RendererPushRenderPass(renderer, &render_pass);
		}
	}
}

internal void
RendererInit(Renderer *renderer, MemoryArena *arena)
{
	renderer->linear_sampler = SamplerInitFilter(VK_FILTER_NEAREST);
	
	m4 capture_projection_matrix = M4Perspective(90.0f, 1.0f, 0.1f, 10.0f);
	
	m4 capture_view_matrices[] = {
		M4LookAt(v3(0.f, 0.f, 0.f), v3( 1.f, 0.f, 0.f), v3(0.f, 1.f, 0.f)), // X+
		M4LookAt(v3(0.f, 0.f, 0.f), v3(-1.f, 0.f, 0.f), v3(0.f, 1.f, 0.f)), // X-
		M4LookAt(v3(0.f, 0.f, 0.f), v3( 0.f, 1.f, 0.f), v3(0.f, 0.f, 1.f)), // Y+
		M4LookAt(v3(0.f, 0.f, 0.f), v3( 0.f,-1.f, 0.f), v3(0.f, 0.f,-1.f)), // Y-
		M4LookAt(v3(0.f, 0.f, 0.f), v3( 0.f, 0.f,-1.f), v3(0.f, 1.f, 0.f)), // Z+
		M4LookAt(v3(0.f, 0.f, 0.f), v3( 0.f, 0.f, 1.f), v3(0.f, 1.f, 0.f)), // Z-
	};
	
	renderer->cubemap_capture_transforms = GPUBufferAllocate(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
															 VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
															 sizeof(m4) * 6);
	
	for(i32 i = 0; i < 6; i++)
	{
		m4 m = M4MultiplyM4(capture_projection_matrix, capture_view_matrices[i]);
		GPUBufferWrite(&renderer->cubemap_capture_transforms, &m, sizeof(m4), sizeof(m4) * i);
	}
	
	typedef struct EnvironmentCubeVertex
	{
		v3 position;
	}
	EnvironmentCubeVertex;
	
	AddVertexBinding(&renderer->environment_cube_vertex_format, sizeof(EnvironmentCubeVertex), VK_VERTEX_INPUT_RATE_VERTEX);
	AddVertexAttribute(&renderer->environment_cube_vertex_format, VK_FORMAT_R32G32B32_SFLOAT, offsetof(EnvironmentCubeVertex, position));
	
	EnvironmentCubeVertex vertices[] = {
		{ { -1.f,  1.f, -1.f } },
		{ { -1.f, -1.f, -1.f } },
		{ {  1.f, -1.f, -1.f } },
		{ {  1.f,  1.f, -1.f } },
		{ { -1.f,  1.f,  1.f } },
		{ { -1.f, -1.f,  1.f } },
		{ {  1.f, -1.f,  1.f } },
		{ {  1.f,  1.f,  1.f } }
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
	
	renderer->environment_hdr_image = ImageFromPath(str8("res/environment_map.hdr"));
	
	renderer->environment_hdr_to_cubemap_program = ShaderProgramInit(sizeof(u64) + sizeof(u32)*4, 2);
	{
		renderer->environment_hdr_to_cubemap_program.stages[0] = ShaderStageLoadFromBytecode(arena, str8("res/hdr_to_environment_cubemap_vertex.spv"), VK_SHADER_STAGE_VERTEX_BIT);
		renderer->environment_hdr_to_cubemap_program.stages[1] = ShaderStageLoadFromBytecode(arena, str8("res/hdr_to_environment_cubemap_fragment.spv"), VK_SHADER_STAGE_FRAGMENT_BIT);
	}
	
	renderer->irradiance_map_program = ShaderProgramInit(sizeof(u64) + sizeof(u32)*4, 2);
	{
		renderer->irradiance_map_program.stages[0] = ShaderStageLoadFromBytecode(arena, str8("res/irradiance_convolution_vertex.spv"), VK_SHADER_STAGE_VERTEX_BIT);
		renderer->irradiance_map_program.stages[1] = ShaderStageLoadFromBytecode(arena, str8("res/irradiance_convolution_fragment.spv"), VK_SHADER_STAGE_FRAGMENT_BIT);
	}
	
	renderer->prefilter_map_program = ShaderProgramInit(sizeof(u64) + sizeof(f32) + sizeof(u32)*3, 2);
	{
		renderer->prefilter_map_program.stages[0] = ShaderStageLoadFromBytecode(arena, str8("res/prefilter_convolution_vertex.spv"), VK_SHADER_STAGE_VERTEX_BIT);
		renderer->prefilter_map_program.stages[1] = ShaderStageLoadFromBytecode(arena, str8("res/prefilter_convolution_fragment.spv"), VK_SHADER_STAGE_FRAGMENT_BIT);
	}
	
	RendererGenerateEnvironmentMap(renderer);
	RendererGenerateEnvironmentProbeFromEnvironmentCubemap(renderer, &renderer->environment_cubemap);
}

internal void
RendererDestroy(Renderer *renderer)
{
	SamplerDestroy(&renderer->linear_sampler);
	
	MeshDestroy(&renderer->environment_cube_mesh);
	GPUBufferDestroy(&renderer->cubemap_capture_transforms);
	
	ImageDestroy(&renderer->environment_hdr_image);
	ImageDestroy(&renderer->environment_cubemap);
	
	ImageDestroy(&renderer->environment_probe.irradiance);
	ImageDestroy(&renderer->environment_probe.prefilter);
	
	ShaderProgramDestroy(&renderer->environment_hdr_to_cubemap_program);
	ShaderProgramDestroy(&renderer->irradiance_map_program);
	ShaderProgramDestroy(&renderer->prefilter_map_program);
}

internal void
RendererBeginFrame(Renderer *renderer)
{
	renderer->present_cmd = BeginGraphicsPresent();
}

internal void
RendererEndFrame(Renderer *renderer)
{
	RendererExecuteRenderPasses(renderer, &renderer->present_cmd);
	EndGraphicsPresent(&renderer->present_cmd);
}
