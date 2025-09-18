
struct export_hdr_pass_context {
	bindless_handle input;
};

internal void RenderPassExportHDRCubemap(RenderState *rs, RenderInfo *render_info, void *context)
{
	CommandBuffer *cmd = &rs->cmd;

	struct export_hdr_pass_context *pass_context = context;
	
	struct {
		u64 transform_buffer;
		u64 vertex_buffer;
		u32 hdr_image_id;
		u32 linear_sampler_id;
	} args;

	args.transform_buffer = core->cubemap_capture_transforms.device_address;
	args.vertex_buffer = core->skybox_mesh.vertex_buffer.device_address;
	args.hdr_image_id = pass_context->input;
	args.linear_sampler_id = core->linear_sampler.bindless.id;

	GraphicsPipelineDef pipeline_def = GraphicsPipelineDefInitDefault(&shaders->hdr_to_environment_cubemap_program);
	pipeline_def.depth_stencil_state.depth_test_enabled = false;
	pipeline_def.depth_stencil_state.depth_write_enabled = false;
	pipeline_def.colour_attachment_count = 1;
	pipeline_def.colour_attachment_formats[0] = VK_FORMAT_R32G32B32A32_SFLOAT;
	pipeline_def.view_mask = 0b111111;

	PipelineState st = FetchGraphicsPipeline(&pipeline_def);

	CmdBindBindless(cmd, st.bind_point, st.layout);
	CmdBindPipeline(cmd, st.bind_point, st.pipeline);

	CmdPushConstants(cmd, st.layout, VK_SHADER_STAGE_ALL_GRAPHICS, sizeof(args), &args);

	MeshBindIndices(&core->skybox_mesh, cmd);
	MeshDrawIndexed(&core->skybox_mesh, cmd);
	
	DebugLog("Created Environment Cubemap.");
}

internal void EnvironmentMapFromHDR(RenderGraph *graph, Image *out, Image *hdr_image)
{
	struct export_hdr_pass_context context = {
		.input = FetchStandardImageViewID(hdr_image).sampled
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
									    NULL, v4(0.f, 0.f, 0.f, 1.f));

	MemoryCopy(render_pass.context, &context, sizeof(context));

	RenderGraphPush(graph, &render_pass);

	RenderPass mipmap_pass = {0};
	mipmap_pass.type = RenderPassType_Mipmap;
	mipmap_pass.mipmap.image = out;

	RenderGraphPush(graph, &mipmap_pass);
}

struct skybox_pass_context {
	bindless_handle skybox;
	Image *target;
	GPUBuffer *frame_data_buffer;
};

internal void RenderPassSkybox(RenderState *rs, RenderInfo *render_info, void *context)
{
	CommandBuffer *cmd = &rs->cmd;

	struct skybox_pass_context *pass_context = context;

	GraphicsPipelineDef pipeline_def = GraphicsPipelineDefInitDefault(&shaders->skybox_program);
	pipeline_def.has_depth_attachment = true;
	pipeline_def.depth_stencil_state.depth_test_enabled = true;
	pipeline_def.depth_stencil_state.depth_write_enabled = false;
	pipeline_def.depth_stencil_state.depth_compare_op = VK_COMPARE_OP_LESS_OR_EQUAL;
	pipeline_def.colour_attachment_count = 1;
	pipeline_def.colour_attachment_formats[0] = pass_context->target->format;

	PipelineState st = FetchGraphicsPipeline(&pipeline_def);

	struct {
		u64 frame_data_buffer;
		u64 vertex_buffer;
		u32 cubemap_id;
		u32 sampler_id;
	} args;

	args.frame_data_buffer = pass_context->frame_data_buffer->device_address;
	args.vertex_buffer = core->skybox_mesh.vertex_buffer.device_address;
	args.cubemap_id = pass_context->skybox;
	args.sampler_id = core->linear_sampler.bindless.id;

	CmdBindBindless(cmd, st.bind_point, st.layout);
	CmdBindPipeline(cmd, st.bind_point, st.pipeline);

	CmdPushConstants(cmd, st.layout, VK_SHADER_STAGE_ALL_GRAPHICS, sizeof(args), &args);

	MeshBindIndices(&core->skybox_mesh, cmd);
	MeshDrawIndexed(&core->skybox_mesh, cmd);
}

struct skybox_renderer_input {
	ImageView *skybox;
	GPUBuffer *frame_data_buffer;
	ImageView *target;
	ImageView *depth;
};

internal void SkyboxRender(RenderGraph *graph,
			   struct skybox_renderer_input *input)
{
	struct skybox_pass_context context = {
		.skybox = input->skybox->bindless.sampled,
		.target = input->target->image,
		.frame_data_buffer = input->frame_data_buffer
	};
	
	RenderPass skybox_pass = {0};
	skybox_pass.type = RenderPassType_Graphics;
	skybox_pass.graphics.Record = RenderPassSkybox;
	skybox_pass.graphics.view_count = 1;
	skybox_pass.graphics.views[0] = input->skybox;
	skybox_pass.graphics.attachment_count = 2;

	skybox_pass.graphics.attachments[0] = RenderingAttachmentInitColour(VK_ATTACHMENT_LOAD_OP_LOAD,
									    input->target,
									    NULL, v4(0.f, 0.f, 0.f, 1.f));
	
	skybox_pass.graphics.attachments[1] = RenderingAttachmentInitDepth(VK_ATTACHMENT_LOAD_OP_LOAD,
									   input->depth,
									   NULL, 1.f, 0);

	MemoryCopy(skybox_pass.context, &context, sizeof(context));

	RenderGraphPush(graph, &skybox_pass);
}
