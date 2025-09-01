
struct export_hdr_pass_context {
	ImageView *input;
	Image *output;
};

internal void RenderPassExportHDRCubemap(RenderState *rs, RenderInfo *render_info, void *context)
{
	CommandBuffer *cmd = &rs->cmd;
	struct export_hdr_pass_context *pass_context = (struct export_hdr_pass_context *)context;
	
	CmdBeginRendering(cmd, render_info);
	{
		struct {
			u64 transform_buffer;
			u32 hdr_image_id;
			u32 linear_sampler_id;
			u32 _padding[2];
		} args;

		args.transform_buffer = core->cubemap_capture_transforms.device_address;
		args.hdr_image_id = pass_context->input->resource_id;
		args.linear_sampler_id = core->linear_sampler.resource_id;

		GraphicsPipelineDef pipeline_def = GraphicsPipelineDefInitDefault(&shaders->hdr_to_environment_cubemap_program,
										  &vertex_formats->vec3);
		pipeline_def.depth_stencil_state.depth_test_enabled = false;
		pipeline_def.depth_stencil_state.depth_write_enabled = false;
		pipeline_def.colour_attachment_count = 1;
		pipeline_def.colour_attachment_formats[0] =
			VK_FORMAT_R32G32B32A32_SFLOAT;
		pipeline_def.view_mask = 0b111111;

		PipelineState st = FetchGraphicsPipeline(&pipeline_def);

		CmdBindBindless(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, st.layout);
		CmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, st.pipeline);

		CmdPushConstants(cmd, st.layout, VK_SHADER_STAGE_ALL_GRAPHICS,
				 sizeof(args), &args, 0);

		MeshBindCmd(&core->skybox_mesh, cmd);
		MeshDrawCmd(&core->skybox_mesh, cmd);
	}
	CmdEndRendering(cmd);

	CmdPrepareForMipmapping(cmd, pass_context->output);
	CmdGenerateMipmaps(cmd, pass_context->output);

	DebugLog("Created Environment Cubemap.");
}

internal void EnvironmentMapFromHDR(RenderGraph *graph, Image *out, Image *hdr_image)
{
	struct export_hdr_pass_context context = {
		.input = FetchStandardImageView(hdr_image),
		.output = out
	};
	
	RenderPass render_pass = {0};
	render_pass.type = RenderPassType_Graphics;
	render_pass.graphics.Record = RenderPassExportHDRCubemap;
	render_pass.graphics.view_mask = 0b111111;
	render_pass.graphics.view_count = 1;
	render_pass.graphics.views[0] = FetchStandardImageView(hdr_image);
	render_pass.graphics.attachment_count = 1;
	render_pass.graphics.attachments[0] = RenderingAttachmentInitColour(VK_ATTACHMENT_LOAD_OP_CLEAR,
									    FetchStandardImageView(out),
									    0, v4(0.f, 0.f, 0.f, 1.f));

	MemoryCopy(render_pass.context, &context, sizeof(context));

	RenderGraphPush(graph, &render_pass);
}

struct skybox_pass_context {
	ImageView *skybox;
};

internal void RenderPassSkybox(RenderState *rs, RenderInfo *render_info, void *context)
{
	CommandBuffer *cmd = &rs->cmd;
	RenderStateFrameData *current_frame = RenderStateGetCurrentFrameData(rs);

	struct skybox_pass_context pass_context = *((struct skybox_pass_context *)context);

	CmdBeginRendering(cmd, render_info);
	{
		GraphicsPipelineDef pipeline_def = GraphicsPipelineDefInitDefault(&shaders->skybox_program,
										  &vertex_formats->vec3);
		pipeline_def.has_depth_attachment = true;
		pipeline_def.depth_stencil_state.depth_test_enabled = true;
		pipeline_def.depth_stencil_state.depth_write_enabled = false;
		pipeline_def.depth_stencil_state.depth_compare_op = VK_COMPARE_OP_LESS_OR_EQUAL;
		pipeline_def.colour_attachment_count = 1;
		pipeline_def.colour_attachment_formats[0] = graphics_device->swapchain.format;

		PipelineState st = FetchGraphicsPipeline(&pipeline_def);

		struct {
			u64 frame_data_buffer;
			u32 cubemap_id;
			u32 sampler_id;
		} args;

		args.frame_data_buffer = current_frame->frame_data_buffer.device_address;
		args.cubemap_id = pass_context.skybox->resource_id;
		args.sampler_id = core->linear_sampler.resource_id;

		CmdBindBindless(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, st.layout);
		CmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, st.pipeline);

		CmdPushConstants(cmd, st.layout, VK_SHADER_STAGE_ALL_GRAPHICS,
				 sizeof(args), &args, 0);

		MeshBindCmd(&core->skybox_mesh, cmd);
		MeshDrawCmd(&core->skybox_mesh, cmd);
	}
	CmdEndRendering(cmd);
}

internal void SkyboxRender(RenderGraph *graph, ImageView *skybox, ImageView *depth)
{
	struct skybox_pass_context context = {
		.skybox = skybox
	};
	
	RenderPass skybox_pass = {0};
	skybox_pass.type = RenderPassType_Graphics;
	skybox_pass.graphics.Record = RenderPassSkybox;
	skybox_pass.graphics.view_count = 1;
	skybox_pass.graphics.views[0] = skybox;
	skybox_pass.graphics.attachment_count = 2;
	skybox_pass.graphics.attachments[0] = RenderingAttachmentInitColour(VK_ATTACHMENT_LOAD_OP_LOAD,
									    GetCurrentSwapchainImageView(&graphics_device->swapchain),
									    0, v4(0.f, 0.f, 0.f, 1.f));
	skybox_pass.graphics.attachments[1] = RenderingAttachmentInitDepth(VK_ATTACHMENT_LOAD_OP_LOAD, depth, 0, 1.f, 0);

	MemoryCopy(skybox_pass.context, &context, sizeof(context));

	RenderGraphPush(graph, &skybox_pass);
}
