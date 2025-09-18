
struct irradiance_pass_context {
	Image *target;
	ImageView *skybox;
};

internal void RenderPassGenerateIrradianceMap(RenderState *rs,
					      RenderInfo *render_info,
					      void *context)
{
	CommandBuffer *cmd = &rs->cmd;

	struct irradiance_pass_context *pass_context = context;
	
	struct {
		u64 transform_buffer;
		u64 vertex_buffer;
		u32 environment_map_id;
		u32 linear_sampler_id;
	} args;

	args.transform_buffer = core->cubemap_capture_transforms.device_address;
	args.vertex_buffer = core->skybox_mesh.vertex_buffer.device_address;
	args.environment_map_id = pass_context->skybox->bindless.sampled;
	args.linear_sampler_id = core->linear_sampler.bindless.id;

	GraphicsPipelineDef pipeline_def = GraphicsPipelineDefInitDefault(&shaders->irradiance_map_program);
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
		
	DebugLog("Created Irradiance Cubemap.");
}

struct prefilter_pass_context {
        f32 roughness;
	ImageView *skybox;
};

internal void RenderPassGeneratePrefilterMap(RenderState *rs,
					     RenderInfo *render_info,
					     void *context)
{
	CommandBuffer *cmd = &rs->cmd;

	struct prefilter_pass_context *pass_context = context;
	
	struct {
		u64 transform_buffer;
		u64 vertex_buffer;
		f32 roughness;
		u32 environment_map_id;
		u32 linear_sampler_id;
	} args;

	args.transform_buffer = core->cubemap_capture_transforms.device_address;
	args.vertex_buffer = core->skybox_mesh.vertex_buffer.device_address;
	args.roughness = pass_context->roughness;
	args.environment_map_id = pass_context->skybox->bindless.sampled;
	args.linear_sampler_id = core->linear_sampler.bindless.id;

	GraphicsPipelineDef pipeline_def = GraphicsPipelineDefInitDefault(&shaders->prefilter_map_program);
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
		
	DebugLog("Created a Prefilter Cubemap mip level.");
}

struct ibl_renderer_input {
	EnvironmentProbe *probe;
	ImageView *skybox;
};

internal void IBLRendererPushIrradiancePass(RenderGraph *graph, struct ibl_renderer_input *input)
{
	struct irradiance_pass_context context = {
		.target = &input->probe->irradiance,
		.skybox = input->skybox
	};
		
	RenderPass render_pass = {0};
	render_pass.type = RenderPassType_Graphics;
	render_pass.graphics.Record = RenderPassGenerateIrradianceMap;
	render_pass.graphics.view_mask = 0b111111;
	render_pass.graphics.view_count = 1;
	render_pass.graphics.views[0] = input->skybox;
	render_pass.graphics.attachment_count = 1;
	render_pass.graphics.attachments[0] = RenderingAttachmentInitColour(VK_ATTACHMENT_LOAD_OP_CLEAR,
									    FetchStandardImageView(&input->probe->irradiance),
									    NULL, v4(0.f, 0.f, 0.f, 1.f));

	MemoryCopy(render_pass.context, &context, sizeof(context));
		
	RenderGraphPush(graph, &render_pass);

	RenderPass mipmap_pass = {0};
	mipmap_pass.type = RenderPassType_Mipmap;
	mipmap_pass.mipmap.image = &input->probe->irradiance;

	RenderGraphPush(graph, &mipmap_pass);
}

internal void IBLRendererPushPrefilterPass(RenderGraph *graph, struct ibl_renderer_input *input)
{
	i32 mipmap_count = input->probe->prefilter.mipmap_count;

	for (i32 mip_level = 0; mip_level < mipmap_count; mip_level++) {
		ImageView *prefilter_view = FetchImageView(&input->probe->prefilter,
							   ImageLayerCount(&input->probe->prefilter), 0,
							   mip_level);

		struct prefilter_pass_context context = {
			.roughness = (f32)(mip_level) / (f32)(mipmap_count - 1),
			.skybox = input->skybox
		};
			
		RenderPass render_pass = {0};
		render_pass.type = RenderPassType_Graphics;
		render_pass.graphics.Record = RenderPassGeneratePrefilterMap;
		render_pass.graphics.view_mask = 0b111111;
		render_pass.graphics.view_count = 1;
		render_pass.graphics.views[0] = input->skybox;
		render_pass.graphics.attachment_count = 1;
		render_pass.graphics.attachments[0] = RenderingAttachmentInitColour(VK_ATTACHMENT_LOAD_OP_CLEAR,
										    prefilter_view,
										    NULL, v4(0.f, 0.f, 0.f, 1.f));

		MemoryCopy(render_pass.context, &context, sizeof(context));

		RenderGraphPush(graph, &render_pass);
	}
}

internal void IBLRendererGenerateEnvironmentProbe(RenderGraph *graph, struct ibl_renderer_input *input)
{
	IBLRendererPushIrradiancePass(graph, input);
	IBLRendererPushPrefilterPass(graph, input);
}
