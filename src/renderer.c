
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
//           [ ] 12. Scene.
//                   --> Camera, lights, etc...
//           [ ] 13. Due to dynamic rendering, there is a lot of data you have to duplicate
//                   between graphics pipelines and render info's (view mask, formats, etc...),
//                   figure out a way to merge this together. Maybe pass render info
//                   into GraphicsPipelineCreate(...)?
//           [ ] 13.5 Final Requirements
//                   --> Texture Asset Manager to stop unfreed images when loading in models.
//                   --> That's it.
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
		args.hdr_image_id = FetchStandardImageView(&renderer->environment_hdr_image)->resource_id;
		args.linear_sampler_id = renderer->linear_sampler.resource_id;
		
		GraphicsPipelineDef pipeline_def = GraphicsPipelineDefInitDefault(&renderer->environment_hdr_to_cubemap_program, &vertex_formats->v3_format);
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
		
		MeshBindCmd(&renderer->environment_cube_mesh, cmd);
		MeshDrawCmd(&renderer->environment_cube_mesh, cmd);
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
	render_pass.graphics.views[0] = FetchStandardImageView(&renderer->environment_hdr_image);
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
		
		MeshBindCmd(&renderer->environment_cube_mesh, cmd);
		MeshDrawCmd(&renderer->environment_cube_mesh, cmd);
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
		
		MeshBindCmd(&renderer->environment_cube_mesh, cmd);
		MeshDrawCmd(&renderer->environment_cube_mesh, cmd);
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
		
		for(i32 i = 0; i < renderer->light_count; i++)
		{
			Light *light = renderer->lights + i;
			
			/*
			GPU_PointLight gpu_point_light = {0};
			gpu_point_light.position.xyz = light->position;
			gpu_point_light.colour.xyz = light->colour;
			gpu_point_light.colour.w = light->intensity;
			gpu_point_light.attenuation = v4(light->range, 0.f, 0.f, 0.f);
			*/
		}
	}
	CmdEndRendering(cmd);
}

/*
				for (int i = 0; i < context.scene->getPointLightCount(); i++)
				{
					cauto &l = context.scene->getPointLights()[i];

					struct
					{
						VkDeviceAddress frameData;
						VkDeviceAddress lights;

						glm::mat4 model;

						uint32_t position_id;
						uint32_t albedo_id;
						uint32_t normal_id;
						uint32_t material_id;
						uint32_t emissive_id;

						uint32_t brdfLUT_id;

						uint32_t textureSampler_id;
						
						uint32_t light_id;
					}
					pc;

					float epsilonIntensity = 0.02f;
					glm::vec3 col = l.getColour().getDisplayColour();
					float lightMax = CalcF::max(CalcF::max(col.r, col.g), col.b);
					float heuristicR = CalcF::sqrt((l.getIntensity() * lightMax) / (l.getFalloff() * epsilonIntensity));

					pc.frameData			= bufAddr(m_frameConstantsBuffer);
					pc.lights				= bufAddr(m_pointLightBuffer);
					pc.position_id			= tex2DIdx(stdView(m_gBuffer.attachments[GBuffer::ATTACHMENT_POSITION]));
					pc.albedo_id			= tex2DIdx(stdView(m_gBuffer.attachments[GBuffer::ATTACHMENT_ALBEDO]));
					pc.normal_id			= tex2DIdx(stdView(m_gBuffer.attachments[GBuffer::ATTACHMENT_NORMAL]));
					pc.material_id			= tex2DIdx(stdView(m_gBuffer.attachments[GBuffer::ATTACHMENT_MATERIAL]));
					pc.emissive_id			= tex2DIdx(stdView(m_gBuffer.attachments[GBuffer::ATTACHMENT_EMISSIVE]));
					pc.brdfLUT_id			= tex2DIdx(stdView(m_brdfLUT));
					pc.textureSampler_id	= smpIdx(m_app->getTextures().getLinearSampler());
					
					pc.model = glm::identity<glm::mat4>()
						* glm::translate(glm::identity<glm::mat4>(), l.getPosition())
						* glm::scale(glm::identity<glm::mat4>(), { heuristicR, heuristicR, heuristicR });

					pc.light_id = i;

					cmd->pushConstants(
						pipelineSt.layout,
						VK_SHADER_STAGE_ALL_GRAPHICS,
						sizeof(pc),
						&pc
					);

					cmd->drawIndexed(m_sphereMesh->getIndexCount());
*/

internal void
RendererRenderPassSkybox(Renderer *renderer, CommandBuffer *cmd, RenderInfo *render_info, void *context)
{
	CmdBeginRendering(cmd, render_info);
	{
		GraphicsPipelineDef pipeline_def = GraphicsPipelineDefInitDefault(&renderer->skybox_program, &vertex_formats->v3_format);
		pipeline_def.depth_stencil_state.depth_test_enabled = true;
		pipeline_def.depth_stencil_state.depth_write_enabled = false;
		pipeline_def.depth_stencil_state.depth_compare_op = VK_COMPARE_OP_LESS_OR_EQUAL;
		pipeline_def.colour_attachment_count = 1;
		pipeline_def.colour_attachment_formats[0] = graphics_device->swapchain.format;
		pipeline_def.has_depth_attachment = true;
		
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
		MeshBindCmd(&renderer->environment_cube_mesh, cmd);
		MeshDrawCmd(&renderer->environment_cube_mesh, cmd);
	}
	CmdEndRendering(cmd);
}

internal void
RendererInit(Renderer *renderer, MemoryArena *arena)
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
	
	renderer->environment_cube_mesh = MeshInit(&vertex_formats->v3_format,
											   ArraySize(vertices), vertices,
											   ArraySize(indices), indices);
	
	renderer->environment_hdr_image = ImageLoadFromPath(str8("res/environment_map.hdr"));
	
	// ---
	
	// NOTE(kp): Load in shaders.
	
	// TODO(kp): Loading in of assets like shaders and images should be done via a seperate asset system.
	
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
	
	renderer->model_program = ShaderProgramInit(sizeof(m4) + sizeof(u32)*8, 2);
	{
		renderer->model_program.stages[0] = ShaderStageLoadFromBytecode(arena, str8("res/model_vertex.spv"), VK_SHADER_STAGE_VERTEX_BIT);
		renderer->model_program.stages[1] = ShaderStageLoadFromBytecode(arena, str8("res/model_fragment.spv"), VK_SHADER_STAGE_FRAGMENT_BIT);
	}
	
	renderer->skybox_program = ShaderProgramInit(sizeof(u32)*4, 2);
	{
		renderer->skybox_program.stages[0] = ShaderStageLoadFromBytecode(arena, str8("res/skybox_vertex.spv"), VK_SHADER_STAGE_VERTEX_BIT);
		renderer->skybox_program.stages[1] = ShaderStageLoadFromBytecode(arena, str8("res/skybox_fragment.spv"), VK_SHADER_STAGE_FRAGMENT_BIT);
	}
	
	renderer->ambient_lighting_program = ShaderProgramInit(sizeof(u64) + sizeof(u32)*4*3, 2);
	{
		renderer->ambient_lighting_program.stages[0] = ShaderStageLoadFromBytecode(arena, str8("res/ambient_lighting_vertex.spv"), VK_SHADER_STAGE_VERTEX_BIT);
		renderer->ambient_lighting_program.stages[1] = ShaderStageLoadFromBytecode(arena, str8("res/ambient_lighting_fragment.spv"), VK_SHADER_STAGE_FRAGMENT_BIT);
	}
	
	renderer->brdf_lut_program = ShaderProgramInit(sizeof(u64) + sizeof(u32)*4*3, 2);
	{
		renderer->brdf_lut_program.stages[0] = ShaderStageLoadFromBytecode(arena, str8("res/brdf_lut_vertex.spv"), VK_SHADER_STAGE_VERTEX_BIT);
		renderer->brdf_lut_program.stages[1] = ShaderStageLoadFromBytecode(arena, str8("res/brdf_lut_fragment.spv"), VK_SHADER_STAGE_FRAGMENT_BIT);
	}
	
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
	
	// ---
	
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
	
	MeshDestroy(&renderer->environment_cube_mesh);
	
	GPUBufferDestroy(&renderer->cubemap_capture_transforms);
	GPUBufferDestroy(&renderer->frame_data_buffer);
	GPUBufferDestroy(&renderer->transform_data_buffer);
	
	ImageDestroy(&renderer->brdf_lut_image);
	ImageDestroy(&renderer->environment_hdr_image);
	ImageDestroy(&renderer->environment_cubemap);
	ImageDestroy(&renderer->environment_probe.irradiance);
	ImageDestroy(&renderer->environment_probe.prefilter);
	
	ShaderProgramDestroy(&renderer->environment_hdr_to_cubemap_program);
	ShaderProgramDestroy(&renderer->irradiance_map_program);
	ShaderProgramDestroy(&renderer->prefilter_map_program);
	ShaderProgramDestroy(&renderer->model_program);
	ShaderProgramDestroy(&renderer->ambient_lighting_program);
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
	frame_data.camera_position = v4(renderer->active_camera->position.x,
									renderer->active_camera->position.y,
									renderer->active_camera->position.z,
									0.f);
	frame_data.window_resolution = v4(1280.f, 720.f, 0.f, 0.f);
	frame_data.time = 0.f;
	
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
RendererEndFrame(Renderer *renderer)
{
	RendererUpdateFrameData(renderer);
	RendererUpdateModelTransformBuffer(renderer);
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
