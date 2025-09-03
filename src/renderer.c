
// TODO: (In order of what to do next to achieve feature parity with magpie C++)
//       [x]  1. Pipeline state caching, two seperate hash
//               tables for layouts and pipelines. Also cache
//               image views automaticlly.
//       [x]  2. Irradiance + Prefilter map generation.
//       [x]  3. Remove combined image-sampler from bindless
//               and add seperate image + sampler tables.
//       [x]  4. Generic hash table implementation.
//       [x]  5. Well commented codebase (self commenting code counts).
//       [x]  6. More Assert(...), DebugLog(...) and DebugLogCrash(...) in the codebase.
//       [x]  7. Investigate how I'm taking up ~100kb of memory in the allocated 32MB?
//               --> RenderPass is just a very big struct, and the renderer has 32
//                   of them at all times.
//       [x]  8. Model loading.
//       [x]  9. Split up files accordingly to make the code easier to manage.
//       [x] 10. Material system.
//       [x] 11. Deferred Rendering.
//       [x] 12. Scene.
//               --> Camera, lights, etc...
//       [x] 13. Final Requirements
//               [x] Asset system to stop unfreed images when loading in models.
//               [x] Shaders should automatically assign their push constants sizes
//                   rather than me having to do it manually.
//               [x] Move render graph into core and command buffer into render context.
//               [x] RenderState shouldn't have any associated functions like
//                   RenderStateGenerateBRDFLookUp(...), rather there should be seperate
//                   renderers for that sort of thing.
//                   --> Same goes for e.g: updating per frame data.
//
//               <<< MAGPIE C++ ENDS HERE >>>
//
//       [ ] 14. Debug renderer (lines, spheres, etc...) (seperate thing)
//       [ ] 15. Text rendering (fonts)
//       [ ] 16. (This applies to all bindless resources.)
//               resource_id should *not* be assigned in the
//               graphics device. In fact, the graphics device
//               should not be managing bindless in the first
//               place, that should be a policy of the renderer.
//               --> Maybe in the future, have a BindlessResources
//                   struct in the high level, that different renderers
//                   can use to manage their bindless resources
//       [ ] 17. Switch to using timeline semaphores over fences for frame synchronisation.
//       [ ] 18. Start going through ideas list in readme.md.

struct geometry_pass_context {
	Renderer *renderer;
	MeshPass *mesh_pass;
};

internal void RenderPassGeometry(RenderState *rs, RenderInfo *render_info, void *context)
{
	CommandBuffer *cmd = &rs->cmd;
	CoreFrameData *current_frame = CoreCurrentFrame();

	struct geometry_pass_context *pass_context = (struct geometry_pass_context *)context;
	Renderer *renderer = pass_context->renderer;
	
	GraphicsPipelineDef pipeline_def = GraphicsPipelineDefInitDefault(&shaders->model_program,
									  &vertex_formats->model);
	{
		pipeline_def.colour_attachment_count = GBufferAttachment_MaxEnum;

		for (i32 i = 0; i < GBufferAttachment_MaxEnum; i++)
			pipeline_def.colour_attachment_formats[i] = renderer->gbuffer.attachments[i].format;

		pipeline_def.has_depth_attachment = true;
	}
	PipelineState st = FetchGraphicsPipeline(&pipeline_def);

	CmdBindBindless(cmd, st.bind_point, st.layout);
	CmdBindPipeline(cmd, st.bind_point, st.pipeline);

	for (IndirectBatch *batch = pass_context->mesh_pass->batches; batch; batch = batch->next) {
		struct {
			u64 frame_data_buffer;
			u64 transform_buffer;
			u64 material_buffer;
			u32 material_id;
			u32 sampler;
		} args;

		args.frame_data_buffer = current_frame->frame_data_buffer.device_address;
		args.transform_buffer = current_frame->object_buffer.device_address;
		args.material_buffer = rs->material_buffer->device_address;
		args.material_id = batch->material_id;
		args.sampler = core->linear_sampler.resource_id;

		CmdPushConstants(cmd, st.layout, VK_SHADER_STAGE_ALL_GRAPHICS, sizeof(args), &args);

		MeshBindCmd(rs->meshes[batch->mesh_id].original, cmd);

		CmdDrawIndexedIndirect(cmd, &current_frame->indirect_buffer,
				       sizeof(VkDrawIndexedIndirectCommand) * batch->first,
				       batch->count,
				       sizeof(VkDrawIndexedIndirectCommand));
	}
}

struct lighting_pass_context {
	Renderer *renderer;
	EnvironmentProbe *probe;
};

internal void RenderPassLighting(RenderState *rs, RenderInfo *render_info, void *context)
{
	struct lighting_pass_context *pass_context = (struct lighting_pass_context *)context;
	Renderer *renderer = pass_context->renderer;
	EnvironmentProbe *probe = pass_context->probe;
	CommandBuffer *cmd = &rs->cmd;
	CoreFrameData *current_frame = CoreCurrentFrame();

	// Ambient Lighting.
	{
		GraphicsPipelineDef pipeline_def = GraphicsPipelineDefInitDefault(&shaders->ambient_lighting_program, 0);
		pipeline_def.depth_stencil_state.depth_test_enabled = false;
		pipeline_def.depth_stencil_state.depth_write_enabled = false;
		pipeline_def.colour_attachment_count = 1;
		pipeline_def.colour_attachment_formats[0] = graphics_device->swapchain.format;
		
		PipelineState st = FetchGraphicsPipeline(&pipeline_def);

		CmdBindBindless(cmd, st.bind_point, st.layout);
		CmdBindPipeline(cmd, st.bind_point, st.pipeline);

		struct {
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

			u32 _padding;
		} args;

		args.frame_data_buffer = current_frame->frame_data_buffer.device_address;

		args.position = FetchStandardImageView(renderer->gbuffer.attachments + GBufferAttachment_Position)->resource_id;
		args.albedo   = FetchStandardImageView(renderer->gbuffer.attachments + GBufferAttachment_Albedo)->resource_id;
		args.normal   = FetchStandardImageView(renderer->gbuffer.attachments + GBufferAttachment_Normal)->resource_id;
		args.material = FetchStandardImageView(renderer->gbuffer.attachments + GBufferAttachment_Material)->resource_id;
		args.emissive = FetchStandardImageView(renderer->gbuffer.attachments + GBufferAttachment_Emissive)->resource_id;

		args.irradiance_map = FetchStandardImageView(&probe->irradiance)->resource_id;
		args.prefilter_map = FetchStandardImageView(&probe->prefilter)->resource_id;
		args.brdf_lut = FetchStandardImageView(&core->brdf_lut_image)->resource_id;

		args.linear_sampler = core->linear_sampler.resource_id;

		CmdPushConstants(cmd, st.layout, VK_SHADER_STAGE_ALL_GRAPHICS, sizeof(args), &args);
		CmdDrawVerticesN(cmd, 3);
	}

	// Direct lighting.
	{
		GraphicsPipelineDef pipeline_def = GraphicsPipelineDefInitDefault(&shaders->direct_lighting_point_program,
										  &vertex_formats->vec3);
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

		CmdBindBindless(cmd, st.bind_point, st.layout);
		CmdBindPipeline(cmd, st.bind_point, st.pipeline);

		MeshBindCmd(&core->light_sphere_mesh, cmd);

		for (u32 i = 0; i < rs->light_count; i++) {
			struct {
				u64 frame_data_buffer;
				u64 light_buffer;
				
				u32 position;
				u32 albedo;
				u32 normal;
				u32 material;
				u32 emissive;
				
				u32 linear_sampler;
				
				u32 _padding[2];
			} args;

			args.frame_data_buffer = current_frame->frame_data_buffer    .device_address;
			args.light_buffer      = current_frame->light_buffer         .device_address;
				
			args.position = FetchStandardImageView(renderer->gbuffer.attachments + GBufferAttachment_Position)->resource_id;
			args.albedo   = FetchStandardImageView(renderer->gbuffer.attachments + GBufferAttachment_Albedo)->resource_id;
			args.normal   = FetchStandardImageView(renderer->gbuffer.attachments + GBufferAttachment_Normal)->resource_id;
			args.material = FetchStandardImageView(renderer->gbuffer.attachments + GBufferAttachment_Material)->resource_id;
			args.emissive = FetchStandardImageView(renderer->gbuffer.attachments + GBufferAttachment_Emissive)->resource_id;
				
			args.linear_sampler = core->linear_sampler.resource_id;

			CmdPushConstants(cmd, st.layout, VK_SHADER_STAGE_ALL_GRAPHICS, sizeof(args), &args);

			MeshDrawCmdID(&core->light_sphere_mesh, cmd, i);
		}
	}
}

// ---

internal void RendererInit(Renderer *renderer)
{
	for (i32 i = 0; i < GBufferAttachment_MaxEnum; i++) {
		renderer->gbuffer.attachments[i] = ImageAlloc2D(graphics_device->swapchain.width, graphics_device->swapchain.height,
								VK_FORMAT_R32G32B32A32_SFLOAT, 1);
		
		renderer->gbuffer.views[i] = FetchStandardImageView(&renderer->gbuffer.attachments[i]);
	}
	
	renderer->gbuffer.depth = ImageAlloc2D(graphics_device->swapchain.width, graphics_device->swapchain.height,
					       graphics_device->depth_format, 1);

	renderer->gbuffer.depth_view = FetchStandardImageView(&renderer->gbuffer.depth);
}

internal void RendererDestroy(Renderer *renderer)
{
	for (i32 i = 0; i < GBufferAttachment_MaxEnum; i++)
		ImageDestroy(renderer->gbuffer.attachments + i);

	ImageDestroy(&renderer->gbuffer.depth);
}

struct deferred_renderer_input {
	Scene *scene;
	Camera *camera;
	EnvironmentProbe *probe;
	MeshPass *mesh_pass;
	ImageView *target;
};

internal void DeferredRenderFrame(Renderer *renderer,
				  RenderGraph *graph,
				  struct deferred_renderer_input *input)
{
	// --- GEOMETRY PASS

	struct geometry_pass_context geometry_context = {
		.renderer = renderer,
		.mesh_pass = input->mesh_pass
	};
	
	RenderPass gbuffer_render_pass = {0};
	gbuffer_render_pass.type = RenderPassType_Graphics;
	gbuffer_render_pass.graphics.Record = RenderPassGeometry;
	gbuffer_render_pass.graphics.attachment_count = GBufferAttachment_MaxEnum + 1;

	for (i32 i = 0; i < GBufferAttachment_MaxEnum; i++)
		gbuffer_render_pass.graphics.attachments[i] = RenderingAttachmentInitColour(VK_ATTACHMENT_LOAD_OP_CLEAR,
											    renderer->gbuffer.views[i],
											    NULL, v4(0.f, 0.f, 0.f, 1.f));

	gbuffer_render_pass.graphics.attachments[GBufferAttachment_MaxEnum] = RenderingAttachmentInitDepth(VK_ATTACHMENT_LOAD_OP_CLEAR,
													   renderer->gbuffer.depth_view,
													   NULL, 1.f, 0);

	MemoryCopy(gbuffer_render_pass.context, &geometry_context, sizeof(geometry_context));
	
	RenderGraphPush(graph, &gbuffer_render_pass);

	// --- LIGHTING PASS

	struct lighting_pass_context lighting_context = {
		.renderer = renderer,
		.probe = input->probe
	};
	
	RenderPass lighting_render_pass = {0};
	lighting_render_pass.type = RenderPassType_Graphics;
	lighting_render_pass.graphics.Record = RenderPassLighting;
	lighting_render_pass.graphics.view_count = GBufferAttachment_MaxEnum + 2;

	for (i32 i = 0; i < GBufferAttachment_MaxEnum; i++)
		lighting_render_pass.graphics.views[i] = FetchStandardImageView(renderer->gbuffer.attachments + i);

	lighting_render_pass.graphics.views[GBufferAttachment_MaxEnum + 0] = FetchStandardImageView(&input->probe->irradiance);
	lighting_render_pass.graphics.views[GBufferAttachment_MaxEnum + 1] = FetchStandardImageView(&input->probe->prefilter);

	lighting_render_pass.graphics.attachment_count = 1;
	lighting_render_pass.graphics.attachments[0] = RenderingAttachmentInitColour(VK_ATTACHMENT_LOAD_OP_CLEAR, input->target,
										     NULL, v4(0.f, 0.f, 0.f, 1.f));

	MemoryCopy(lighting_render_pass.context, &lighting_context, sizeof(lighting_context));

	RenderGraphPush(graph, &lighting_render_pass);
}
