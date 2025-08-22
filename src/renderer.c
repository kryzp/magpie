
// TODO(kp): (In order of what to do next to achieve feature parity with magpie C++)
//           [x]  1. Pipeline state caching, two seperate hash
//                   tables for layouts and pipelines. Also cache
//                   image views automaticlly.
//           [x]  2. Irradiance + Prefilter map generation.
//           [x]  3. Remove combined image-sampler from bindless
//                   and add seperate image + sampler tables.
//           [x]  4. Generic hash table implementation.
//           [x]  5. Well commented codebase (self commenting code counts).
//           [x]  6. More Assert(...), DebugLog(...) and DebugLogCrash(...) in the codebase.
//           [x]  7. Investigate how I'm taking up ~100kb of memory in the allocated 32MB?
//                   --> RenderPass is just a very big struct, and the renderer has 32
//                       of them at all times.
//           [x]  8. Model loading.
//           [x]  9. Split up files accordingly to make the code easier to manage.
//           [x] 10. Material system.
//           [x] 11. Deferred Rendering.
//           [x] 12. Scene.
//                   --> Camera, lights, etc...
//           [ ] 13 Final Requirements
//                   [x] Asset system to stop unfreed images when loading in models.
//                   [ ] Shaders should automatically assign their push constants sizes
//                       rather than me having to do it manually.
//
//                   <<< MAGPIE C++ ENDS HERE >>>
//
//           [ ] 14. Debug renderer (lines, spheres, etc...) (seperate thing)
//           [ ] 15. Text rendering (fonts)
//           [ ] 16. (This applies to all bindless resources.)
//                   resource_id should *not* be assigned in the
//                   graphics device. In fact, the graphics device
//                   should not be managing bindless in the first
//                   place, that should be a policy of the renderer.
//                   --> Maybe in the future, have a BindlessResources
//                       struct in the high level, that different renderers
//                       can use to manage their bindless resources
//           [ ] 17. Mipmap generation should happen automatically when executing render passes.
//                   --> When that is achieved, automatically call CmdBeginRendering(...) and
//                       CmdEndRendering(...) around the Record(...) function, since mipmaps
//                       are pretty much the only reason I don't already do that.
//           [ ] 18. Switch to using timeline semaphores over fences for frame synchronisation.
//           [ ] 19. Start going through ideas list in readme.md.

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
RendererPushRenderPass(Renderer *renderer, RenderPass *pass)
{
	Assert(renderer->pass_count < ArraySize(renderer->passes) && "Cannot push more render passes.");
	
	renderer->passes[renderer->pass_count] = *pass;
	renderer->pass_count++;
}

internal void
RendererRenderPassGenerateBRDFLookUp(Renderer *renderer, CommandBuffer *cmd, RenderInfo *render_info, void *context)
{
	CmdBeginRendering(cmd, render_info);
	{
		GraphicsPipelineDef pipeline_def = GraphicsPipelineDefInitDefault(&renderer->brdf_lut_program, 0);
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
RendererGenerateBRDFLookUp(Renderer *renderer)
{
	renderer->brdf_lut_image = ImageAllocate(512, 512, 1,
											 VK_FORMAT_R32G32_SFLOAT,
											 VK_IMAGE_VIEW_TYPE_2D,
											 VK_IMAGE_TILING_OPTIMAL,
											 1,
											 VK_SAMPLE_COUNT_1_BIT,
											 false, false);
	
	RenderPass render_pass = {0};
	render_pass.type = RenderPassType_Graphics;
	render_pass.graphics.Record = RendererRenderPassGenerateBRDFLookUp;
	render_pass.graphics.attachment_count = 1;
	render_pass.graphics.attachments[0] = RenderingAttachmentInitColour(VK_ATTACHMENT_LOAD_OP_CLEAR,
																		FetchStandardImageView(&renderer->brdf_lut_image),
																		0, v4(0.f, 0.f, 0.f, 1.f));
	
	RendererPushRenderPass(renderer, &render_pass);
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
		args.hdr_image_id = FetchStandardImageView(&renderer->environment_hdr_texture)->resource_id;
		args.linear_sampler_id = renderer->linear_sampler.resource_id;
		
		GraphicsPipelineDef pipeline_def = GraphicsPipelineDefInitDefault(&renderer->hdr_to_environment_cubemap_program, &vertex_formats->v3_format);
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
		
		MeshBindCmd(&renderer->skybox_mesh, cmd);
		MeshDrawCmd(&renderer->skybox_mesh, cmd);
	}
	CmdEndRendering(cmd);
	
	CmdPrepareForMipmapping(cmd, &renderer->environment_cubemap);
	CmdGenerateMipmaps(cmd, &renderer->environment_cubemap);
	
	DebugLog("Created Environment Cubemap.");
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
	render_pass.graphics.views[0] = FetchStandardImageView(&renderer->environment_hdr_texture);
	render_pass.graphics.attachment_count = 1;
	render_pass.graphics.attachments[0] = RenderingAttachmentInitColour(VK_ATTACHMENT_LOAD_OP_CLEAR,
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
		
		GraphicsPipelineDef pipeline_def = GraphicsPipelineDefInitDefault(&renderer->irradiance_map_program, &vertex_formats->v3_format);
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
		
		MeshBindCmd(&renderer->skybox_mesh, cmd);
		MeshDrawCmd(&renderer->skybox_mesh, cmd);
	}
	CmdEndRendering(cmd);
	
	CmdPrepareForMipmapping(cmd, &renderer->environment_probe.irradiance);
	CmdGenerateMipmaps(cmd, &renderer->environment_probe.irradiance);
	
	DebugLog("Created Irradiance Cubemap.");
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
		
		GraphicsPipelineDef pipeline_def = GraphicsPipelineDefInitDefault(&renderer->prefilter_map_program, &vertex_formats->v3_format);
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
		
		MeshBindCmd(&renderer->skybox_mesh, cmd);
		MeshDrawCmd(&renderer->skybox_mesh, cmd);
	}
	CmdEndRendering(cmd);
	
	DebugLog("Created a Prefilter Cubemap mip level.");
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
		render_pass.graphics.attachments[0] = RenderingAttachmentInitColour(VK_ATTACHMENT_LOAD_OP_CLEAR,
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
													   ImageLayerCount(&renderer->environment_probe.prefilter),
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
			render_pass.graphics.attachments[0] = RenderingAttachmentInitColour(VK_ATTACHMENT_LOAD_OP_CLEAR,
																				prefilter_view,
																				0, v4(0.f, 0.f, 0.f, 1.f));
			
			RendererPushRenderPass(renderer, &render_pass);
		}
	}
}

internal void
RendererRenderPassRenderModel(Renderer *renderer, CommandBuffer *cmd, RenderInfo *render_info, void *context)
{
	CmdBeginRendering(cmd, render_info);
	{
		GraphicsPipelineDef pipeline_def = GraphicsPipelineDefInitDefault(&renderer->model_program, &vertex_formats->model_format);
		
		pipeline_def.colour_attachment_count = GBufferAttachment_MaxEnum;
		
		for(i32 i = 0; i < GBufferAttachment_MaxEnum; i++)
		{
			pipeline_def.colour_attachment_formats[i] = renderer->gbuffer.attachments[i].format;
		}
		
		pipeline_def.has_depth_attachment = true;
		
		PipelineState st = FetchGraphicsPipeline(&pipeline_def);
		
		CmdBindBindless(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, st.layout);
		CmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, st.pipeline);
		
		for(i32 i = 0; i < renderer->render_call_count; i++)
		{
			RenderCall *call = renderer->render_calls + i;
			
			struct
			{
				u64 frame_data_buffer;
				u64 transform_buffer;
				
				u32 diffuse;
				u32 normal;
				u32 emissive;
				u32 mr;
				u32 ambient;
				
				u32 sampler;
				
				u32 _padding[2];
			}
			args;
			
			args.frame_data_buffer = renderer->frame_data_buffer.device_address;
			args.transform_buffer = renderer->transform_data_buffer.device_address;
			
			args.diffuse  = call->material->diffuse;
			args.normal   = call->material->normal;
			args.emissive = call->material->emissive;
			args.mr       = call->material->mr;
			args.ambient  = call->material->ambient;
			
			args.sampler = renderer->linear_sampler.resource_id;
			
			CmdPushConstants(cmd, st.layout, VK_SHADER_STAGE_ALL_GRAPHICS, sizeof(args), &args, 0);
			
			MeshBindCmd(call->mesh, cmd);
			MeshDrawCmd(call->mesh, cmd);
		}
	}
	CmdEndRendering(cmd);
}

internal void
RendererRenderPassLighting(Renderer *renderer, CommandBuffer *cmd, RenderInfo *render_info, void *context)
{
	CmdBeginRendering(cmd, render_info);
	{
		// NOTE(kp): Ambient Lighting.
		{
			GraphicsPipelineDef pipeline_def = GraphicsPipelineDefInitDefault(&renderer->ambient_lighting_program, 0);
			pipeline_def.depth_stencil_state.depth_test_enabled = false;
			pipeline_def.depth_stencil_state.depth_write_enabled = false;
			pipeline_def.colour_attachment_count = 1;
			pipeline_def.colour_attachment_formats[0] = graphics_device->swapchain.format;
			
			PipelineState st = FetchGraphicsPipeline(&pipeline_def);
			
			CmdBindBindless(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, st.layout);
			CmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, st.pipeline);
			
			struct
			{
				u64 frame_data_buffer;
				
				u32 position;
				u32 albedo;
				u32 normal;
				u32 material;
				u32 emissive;
				
				u32 irradiance_map;
				u32 prefilter_map;
				u32 brdf_lut;
				
				u32 linear_sampler;
				
				u32 _padding[3];
			}
			args;
			
			args.frame_data_buffer = renderer->frame_data_buffer.device_address;
			
			args.position = FetchStandardImageView(renderer->gbuffer.attachments + GBufferAttachment_Position)->resource_id;
			args.albedo   = FetchStandardImageView(renderer->gbuffer.attachments + GBufferAttachment_Albedo)->resource_id;
			args.normal   = FetchStandardImageView(renderer->gbuffer.attachments + GBufferAttachment_Normal)->resource_id;
			args.material = FetchStandardImageView(renderer->gbuffer.attachments + GBufferAttachment_Material)->resource_id;
			args.emissive = FetchStandardImageView(renderer->gbuffer.attachments + GBufferAttachment_Emissive)->resource_id;
			
			args.irradiance_map = FetchStandardImageView(&renderer->environment_probe.irradiance)->resource_id;
			args.prefilter_map  = FetchStandardImageView(&renderer->environment_probe.prefilter)->resource_id;
			args.brdf_lut       = FetchStandardImageView(&renderer->brdf_lut_image)->resource_id;
			
			args.linear_sampler = renderer->linear_sampler.resource_id;
			
			CmdPushConstants(cmd, st.layout, VK_SHADER_STAGE_ALL_GRAPHICS, sizeof(args), &args, 0);
			
			CmdDrawVerticesN(cmd, 3);
		}
		
		// NOTE(kp): Direct lighting.
		{
			GraphicsPipelineDef pipeline_def = GraphicsPipelineDefInitDefault(&renderer->direct_lighting_point_program, &vertex_formats->v3_format);
			pipeline_def.depth_stencil_state.depth_test_enabled = false;
			pipeline_def.depth_stencil_state.depth_write_enabled = false;
			pipeline_def.cull_mode = VK_CULL_MODE_FRONT_BIT;
			pipeline_def.colour_attachment_count = 1;
			pipeline_def.colour_attachment_formats[0] = graphics_device->swapchain.format;
			pipeline_def.blend_state.enabled = true;
			pipeline_def.blend_state.colour.op = VK_BLEND_OP_ADD;
			pipeline_def.blend_state.colour.dst = VK_BLEND_FACTOR_ONE;
			pipeline_def.blend_state.colour.src = VK_BLEND_FACTOR_ONE;
			
			PipelineState st = FetchGraphicsPipeline(&pipeline_def);
			
			CmdBindBindless(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, st.layout);
			CmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, st.pipeline);
			
			MeshBindCmd(&renderer->light_sphere_mesh, cmd);
			
			for(i32 i = 0; i < renderer->light_count; i++)
			{
				Light *light = renderer->lights + i;
				
				f32 epsilon_intensity = .1f;
				f32 light_max = V3MaxValue(light->colour);
				f32 heuristic_radius = SquareRoot((light->intensity * light_max) / (light->falloff * epsilon_intensity));
				
				m4 transform = M4Transform(light->position,
										   QuatInitIdentity(),
										   v3u(heuristic_radius),
										   v3u(0.f));
				
				struct
				{
					u64 frame_data;
					u64 lights;
					
					m4 transform;
					
					u32 light_id;
					
					u32 position;
					u32 albedo;
					u32 normal;
					u32 material;
					u32 emissive;
					
					u32 linear_sampler;
					
					u32 _padding;
				}
				args;
				
				args.frame_data = renderer->frame_data_buffer.device_address;
				args.lights = renderer->light_buffer.device_address;
				
				args.transform = transform;
				
				args.light_id = i;
				
				args.position = FetchStandardImageView(renderer->gbuffer.attachments + GBufferAttachment_Position)->resource_id;
				args.albedo   = FetchStandardImageView(renderer->gbuffer.attachments + GBufferAttachment_Albedo)->resource_id;
				args.normal   = FetchStandardImageView(renderer->gbuffer.attachments + GBufferAttachment_Normal)->resource_id;
				args.material = FetchStandardImageView(renderer->gbuffer.attachments + GBufferAttachment_Material)->resource_id;
				args.emissive = FetchStandardImageView(renderer->gbuffer.attachments + GBufferAttachment_Emissive)->resource_id;
				
				args.linear_sampler = renderer->linear_sampler.resource_id;
				
				CmdPushConstants(cmd, st.layout, VK_SHADER_STAGE_ALL_GRAPHICS, sizeof(args), &args, 0);
				
				MeshDrawCmd(&renderer->light_sphere_mesh, cmd);
			}
		}
	}
	CmdEndRendering(cmd);
}

internal void
RendererRenderPassSkybox(Renderer *renderer, CommandBuffer *cmd, RenderInfo *render_info, void *context)
{
	CmdBeginRendering(cmd, render_info);
	{
		GraphicsPipelineDef pipeline_def = GraphicsPipelineDefInitDefault(&renderer->skybox_program, &vertex_formats->v3_format);
		pipeline_def.has_depth_attachment = true;
		pipeline_def.depth_stencil_state.depth_test_enabled = true;
		pipeline_def.depth_stencil_state.depth_write_enabled = false;
		pipeline_def.depth_stencil_state.depth_compare_op = VK_COMPARE_OP_LESS_OR_EQUAL;
		pipeline_def.colour_attachment_count = 1;
		pipeline_def.colour_attachment_formats[0] = graphics_device->swapchain.format;
		
		PipelineState st = FetchGraphicsPipeline(&pipeline_def);
		
		struct
		{
			u64 frame_data_buffer;
			u32 cubemap_id;
			u32 sampler_id;
		}
		args;
		
		args.frame_data_buffer = renderer->frame_data_buffer.device_address;
		args.cubemap_id = FetchStandardImageView(&renderer->environment_cubemap)->resource_id;
		args.sampler_id = renderer->linear_sampler.resource_id;
		
		CmdBindBindless(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, st.layout);
		CmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, st.pipeline);
		CmdPushConstants(cmd, st.layout, VK_SHADER_STAGE_ALL_GRAPHICS, sizeof(args), &args, 0);
		MeshBindCmd(&renderer->skybox_mesh, cmd);
		MeshDrawCmd(&renderer->skybox_mesh, cmd);
	}
	CmdEndRendering(cmd);
}

// NOTE(kp): https://songho.ca/opengl/gl_sphere.html
// TODO(kp): Use a more efficient sphere shape like an ICOSPHERE or CUBESPHERE.
void RendererCreateUnitSphereMesh(Renderer *renderer, MemoryArena *arena)
{
	ScratchArena scratch = GetScratch(arena);
	
	u32 sector_count = 10;
	u32 stack_count = 10;
	
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
	
	for(i32 i = 0; i < stack_count; i++)
	{
		u16 k1 = i  * (sector_count + 1); // NOTE(kp): Current stack.
		u16 k2 = k1 + (sector_count + 1); // NOTE(kp): Next stack.
		
		for(i32 j = 0; j < sector_count; j++, k1++, k2++)
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
	
	renderer->light_sphere_mesh = MeshInit(&vertex_formats->v3_format,
										   vertex_count, vertices,
										   index_count, indices);
	
	ReleaseScratch(&scratch);
}

internal void
RendererLoadShaders(Renderer *renderer, MemoryArena *arena)
{
	struct
	{
		String8 vert;
		String8 frag;
		ShaderProgram *target;
	}
	graphics_shaders[] =
	{
		{ str8("res/brdf_lut_vertex.spv"),                    str8("res/brdf_lut_fragment.spv"),                    &renderer->brdf_lut_program },
		{ str8("res/hdr_to_environment_cubemap_vertex.spv"),  str8("res/hdr_to_environment_cubemap_fragment.spv"),  &renderer->hdr_to_environment_cubemap_program },
		{ str8("res/irradiance_convolution_vertex.spv"),      str8("res/irradiance_convolution_fragment.spv"),      &renderer->irradiance_map_program },
		{ str8("res/prefilter_convolution_vertex.spv"),       str8("res/prefilter_convolution_fragment.spv"),       &renderer->prefilter_map_program },
		{ str8("res/model_vertex.spv"),                       str8("res/model_fragment.spv"),                       &renderer->model_program },
		{ str8("res/skybox_vertex.spv"),                      str8("res/skybox_fragment.spv"),                      &renderer->skybox_program },
		{ str8("res/ambient_lighting_vertex.spv"),            str8("res/ambient_lighting_fragment.spv"),            &renderer->ambient_lighting_program },
		{ str8("res/direct_lighting_point_vertex.spv"),       str8("res/direct_lighting_point_fragment.spv") ,      &renderer->direct_lighting_point_program },
	};
	
	for(i32 i = 0; i < ArraySize(graphics_shaders); i++)
	{
		String8 files[] = { graphics_shaders[i].vert, graphics_shaders[i].frag };
		(*graphics_shaders[i].target) = ShaderProgramInit(arena, 2, files);
	}
}

internal void
RendererInit(Renderer *renderer, Assets *assets, MemoryArena *arena)
{
	renderer->linear_sampler = SamplerInitFilter(VK_FILTER_NEAREST);
	
	// ---
	
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
	
	renderer->cubemap_capture_transforms = GPUBufferAllocate(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
															 VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
															 sizeof(m4) * 6);
	
	for(i32 i = 0; i < 6; i++)
	{
		m4 m = M4MultiplyM4(capture_projection_matrix, capture_view_matrices[i]);
		GPUBufferWrite(&renderer->cubemap_capture_transforms, &m, sizeof(m4), sizeof(m4) * i);
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
	
	renderer->skybox_mesh = MeshInit(&vertex_formats->v3_format,
									 ArraySize(vertices), vertices,
									 ArraySize(indices), indices);
	
	u32 environment_asset_handle = AssetsLoadTexture(assets, str8("res/environment_map.hdr"));
	renderer->environment_hdr_texture = assets->textures[environment_asset_handle].image;
	
	// ---
	
	// NOTE(kp): G-buffer.
	
	for(i32 i = 0; i < GBufferAttachment_MaxEnum; i++)
	{
		renderer->gbuffer.attachments[i] = ImageAllocate(graphics_device->swapchain.width,
														 graphics_device->swapchain.height,
														 1,
														 VK_FORMAT_R32G32B32A32_SFLOAT,
														 VK_IMAGE_VIEW_TYPE_2D,
														 VK_IMAGE_TILING_OPTIMAL,
														 1,
														 VK_SAMPLE_COUNT_1_BIT,
														 false, false);
	}
	
	renderer->gbuffer.depth = ImageAllocate(graphics_device->swapchain.width,
											graphics_device->swapchain.height,
											1,
											graphics_device->depth_format,
											VK_IMAGE_VIEW_TYPE_2D,
											VK_IMAGE_TILING_OPTIMAL,
											1,
											VK_SAMPLE_COUNT_1_BIT,
											false, false);
	
	// ---
	
	renderer->frame_data_buffer = GPUBufferAllocate(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
													VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
													sizeof(GPU_FrameData));
	
	renderer->transform_data_buffer = GPUBufferAllocate(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
														VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
														sizeof(GPU_TransformData));
	
	renderer->light_buffer = GPUBufferAllocate(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
											   VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
											   sizeof(GPU_Light) * ArraySize(renderer->lights));
	
	// ---
	
	RendererLoadShaders(renderer, arena);
	RendererCreateUnitSphereMesh(renderer, arena);
	RendererGenerateBRDFLookUp(renderer);
	RendererGenerateEnvironmentMap(renderer);
	RendererGenerateEnvironmentProbeFromEnvironmentCubemap(renderer, &renderer->environment_cubemap);
}

internal void
RendererDestroy(Renderer *renderer)
{
	SamplerDestroy(&renderer->linear_sampler);
	
	for(i32 i = 0; i < GBufferAttachment_MaxEnum; i++)
	{
		ImageDestroy(renderer->gbuffer.attachments + i);
	}
	
	ImageDestroy(&renderer->gbuffer.depth);
	
	MeshDestroy(&renderer->light_sphere_mesh);
	MeshDestroy(&renderer->skybox_mesh);
	
	GPUBufferDestroy(&renderer->cubemap_capture_transforms);
	GPUBufferDestroy(&renderer->frame_data_buffer);
	GPUBufferDestroy(&renderer->transform_data_buffer);
	GPUBufferDestroy(&renderer->light_buffer);
	
	ImageDestroy(&renderer->brdf_lut_image);
	ImageDestroy(&renderer->environment_cubemap);
	ImageDestroy(&renderer->environment_probe.irradiance);
	ImageDestroy(&renderer->environment_probe.prefilter);
	
	ShaderProgramDestroy(&renderer->ambient_lighting_program);
	ShaderProgramDestroy(&renderer->direct_lighting_point_program);
	ShaderProgramDestroy(&renderer->model_program);
	ShaderProgramDestroy(&renderer->hdr_to_environment_cubemap_program);
	ShaderProgramDestroy(&renderer->irradiance_map_program);
	ShaderProgramDestroy(&renderer->prefilter_map_program);
	ShaderProgramDestroy(&renderer->skybox_program);
	ShaderProgramDestroy(&renderer->brdf_lut_program);
}

internal void
RendererBeginFrame(Renderer *renderer)
{
	renderer->render_call_count = 0;
	renderer->light_count = 0;
	
	renderer->present_cmd = BeginGraphicsPresent();
}

internal void
RendererUpdateFrameData(Renderer *renderer)
{
	GPU_FrameData frame_data = {0};
	frame_data.view = renderer->active_camera->view;
	frame_data.projection = renderer->active_camera->projection;
	frame_data.view_projection = M4MultiplyM4(frame_data.projection, frame_data.view);
	frame_data.view_projection_no_translation = M4MultiplyM4(frame_data.projection, M4RemoveTranslation(frame_data.view));
	frame_data.inv_view = M4Inverse(frame_data.view);
	frame_data.inv_projection = M4Inverse(frame_data.projection);
	frame_data.camera_position.xyz = renderer->active_camera->position;
	frame_data.window_resolution.x = platform->window_pixel_width;
	frame_data.window_resolution.y = platform->window_pixel_height;
	frame_data.time = GetTotalElapsedSecondsF();
	
	GPUBufferWrite(&renderer->frame_data_buffer, &frame_data, sizeof(GPU_FrameData), 0);
}

internal void
RendererUpdateModelTransformBuffer(Renderer *renderer)
{
	for(i32 i = 0; i < renderer->render_call_count; i++)
	{
		RenderCall *call = renderer->render_calls + i;
		
		GPU_TransformData transform_data = {0};
		transform_data.model_matrix = call->transform;
		transform_data.normal_matrix = M4Inverse(M4Transpose(transform_data.model_matrix));
		
		GPUBufferWrite(&renderer->transform_data_buffer, &transform_data, sizeof(GPU_TransformData), 0);
	}
}

internal void
RendererUpdateLightBuffer(Renderer *renderer)
{
	for(i32 i = 0; i < renderer->light_count; i++)
	{
		Light *light = renderer->lights + i;
		
		GPU_Light gpu_light = {0};
		gpu_light.position.xyz    = light->position;
		gpu_light.colour.xyz      = light->colour;
		gpu_light.colour.w        = light->intensity;
		gpu_light.attenuation.xyz = v3(light->falloff, 0.f, 0.f);
		
		GPUBufferWrite(&renderer->light_buffer, &gpu_light, sizeof(GPU_Light), 0);
	}
}

internal void
RendererEndFrame(Renderer *renderer)
{
	RendererUpdateFrameData(renderer);
	RendererUpdateModelTransformBuffer(renderer);
	RendererUpdateLightBuffer(renderer);
	{
		// --- GEOMETRY PASS
		
		RenderPass gbuffer_render_pass = {0};
		gbuffer_render_pass.type = RenderPassType_Graphics;
		gbuffer_render_pass.graphics.Record = RendererRenderPassRenderModel;
		gbuffer_render_pass.graphics.attachment_count = GBufferAttachment_MaxEnum + 1;
		
		for(i32 i = 0; i < GBufferAttachment_MaxEnum; i++)
		{
			gbuffer_render_pass.graphics.attachments[i] =
				RenderingAttachmentInitColour(VK_ATTACHMENT_LOAD_OP_CLEAR,
											  FetchStandardImageView(renderer->gbuffer.attachments + i),
											  0, v4(0.f, 0.f, 0.f, 1.f));
		}
		
		gbuffer_render_pass.graphics.attachments[GBufferAttachment_MaxEnum] =
			RenderingAttachmentInitDepth(VK_ATTACHMENT_LOAD_OP_CLEAR,
										 FetchStandardImageView(&renderer->gbuffer.depth),
										 0, 1.f, 0);
		
		RendererPushRenderPass(renderer, &gbuffer_render_pass);
		
		// --- LIGHTING PASS
		
		RenderPass lighting_render_pass = {0};
		lighting_render_pass.type = RenderPassType_Graphics;
		lighting_render_pass.graphics.Record = RendererRenderPassLighting;
		lighting_render_pass.graphics.view_count = GBufferAttachment_MaxEnum + 2;
		
		for(i32 i = 0; i < GBufferAttachment_MaxEnum; i++)
		{
			lighting_render_pass.graphics.views[i] = FetchStandardImageView(renderer->gbuffer.attachments + i);
		}
		
		lighting_render_pass.graphics.views[GBufferAttachment_MaxEnum + 0] = FetchStandardImageView(&renderer->environment_probe.irradiance);
		lighting_render_pass.graphics.views[GBufferAttachment_MaxEnum + 1] = FetchStandardImageView(&renderer->environment_probe.prefilter);
		
		lighting_render_pass.graphics.attachment_count = 1;
		lighting_render_pass.graphics.attachments[0] = RenderingAttachmentInitColour(VK_ATTACHMENT_LOAD_OP_CLEAR,
																					 GetCurrentSwapchainImageView(&graphics_device->swapchain),
																					 0, v4(0.f, 0.f, 0.f, 1.f));
		
		RendererPushRenderPass(renderer, &lighting_render_pass);
		
		// --- SKYBOX
		
		RenderPass skybox_pass = {0};
		skybox_pass.type = RenderPassType_Graphics;
		skybox_pass.graphics.Record = RendererRenderPassSkybox;
		skybox_pass.graphics.view_count = 1;
		skybox_pass.graphics.views[0] = FetchStandardImageView(&renderer->environment_cubemap);
		skybox_pass.graphics.attachment_count = 2;
		skybox_pass.graphics.attachments[0] = RenderingAttachmentInitColour(VK_ATTACHMENT_LOAD_OP_LOAD,
																			GetCurrentSwapchainImageView(&graphics_device->swapchain),
																			0, v4(0.f, 0.f, 0.f, 1.f));
		skybox_pass.graphics.attachments[1] = RenderingAttachmentInitDepth(VK_ATTACHMENT_LOAD_OP_LOAD,
																		   FetchStandardImageView(&renderer->gbuffer.depth),
																		   0, 1.f, 0);
		
		RendererPushRenderPass(renderer, &skybox_pass);
	}
	RendererExecuteRenderPasses(renderer, &renderer->present_cmd);
	EndGraphicsPresent(&renderer->present_cmd);
}

internal void
RendererSetCamera(Renderer *renderer, Camera *camera)
{
	renderer->active_camera = camera;
}

internal void
RendererPushLight(Renderer *renderer, Light *light)
{
	Assert(renderer->light_count < ArraySize(renderer->lights) && "Cannot push more lights.");
	
	renderer->lights[renderer->light_count] = *light;
	renderer->light_count++;
}

internal void
RendererPushCall(Renderer *renderer, RenderCall *call)
{
	Assert(renderer->render_call_count < ArraySize(renderer->render_calls) && "Cannot push more render calls.");
	
	renderer->render_calls[renderer->render_call_count] = *call;
	renderer->render_call_count++;
}
