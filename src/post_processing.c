
struct post_processing_input {
	f32 exposure;
	Image *input;
	Image *output;
};

internal void RenderPassHDRTonemapping(RenderState *rs, void *context)
{
	CommandBuffer *cmd = &rs->cmd;
	
	struct post_processing_input *pass_context = (struct post_processing_input *)context;

	ComputePipelineDef pipeline_def = ComputePipelineDefInit(&shaders->hdr_tonemapping_program);
	PipelineState pipeline_st = FetchComputePipeline(&pipeline_def);

	CmdBindBindless(cmd, pipeline_st.bind_point, pipeline_st.layout);
	CmdBindPipeline(cmd, pipeline_st.bind_point, pipeline_st.pipeline);

	struct {
		u32 width;
		u32 height;
		f32 exposure;
		u32 image_id;
	} args;

	args.width = pass_context->input->width;
	args.height = pass_context->input->height;
	args.exposure = pass_context->exposure;
	args.image_id = FetchStandardImageView(pass_context->output)->resource_id;
	
	CmdPushConstants(cmd, pipeline_st.layout, VK_SHADER_STAGE_COMPUTE_BIT, sizeof(args), &args);

	CmdDispatch(cmd, pass_context->output->width, pass_context->output->height, 1);
}

internal void PostProcessingPass(RenderGraph *graph, struct post_processing_input *input)
{
	RenderPass compute_pass = {0};
	compute_pass.type = RenderPassType_Compute;
	compute_pass.compute.Record = RenderPassHDRTonemapping;
	compute_pass.compute.read_only_image_count = 1;
	compute_pass.compute.read_only_images[0] = input->input;
	compute_pass.compute.rw_image_count = 1;
	compute_pass.compute.rw_images[0] = input->output;

	MemoryCopy(compute_pass.context, input, sizeof(struct post_processing_input));

	RenderGraphPush(graph, &compute_pass);
}
