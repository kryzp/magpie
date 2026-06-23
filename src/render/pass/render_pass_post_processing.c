
static R_PASS_RECORD_DEF(R_PostProcessingPassFn)
{
	G_CmdBuffer *cmd = ctx->cmd;
	
	const R_PostProcessingPassData *user_data = ctx->user_data;

	G_ComputePipelineDef pipeline_def = G_ComputePipelineDefInit(user_data->shader);

	G_PipelineSt pipeline_st = G_DeviceFetchComputePipeline(ctx->device, &pipeline_def);
	
	struct
	{
		u32 width;
		u32 height;
		f32 exposure;
		u32 input_texture;
		u32 output_texture;
	}
	args;

	G_TextureKey input_key  = R_GraphResolveTexture(ctx->graph, user_data->input);
	G_TextureKey output_key = R_GraphResolveTexture(ctx->graph, user_data->output);
	
	G_Texture *input_texture  = G_DeviceTextureFromKey(ctx->device, input_key);
	G_Texture *output_texture = G_DeviceTextureFromKey(ctx->device, output_key);

	G_TextureViewKey input_view  = G_DeviceTextureViewAuto(ctx->device, input_key);
	G_TextureViewKey output_view = G_DeviceTextureViewAuto(ctx->device, output_key);
	
	args.width          = input_texture->width;
	args.height         = input_texture->height;
	args.exposure       = user_data->exposure;
	args.input_texture  = G_DeviceTextureViewBindless(ctx->device, input_view);
	args.output_texture = G_DeviceTextureViewBindless(ctx->device, output_view);
	
	G_CmdBindBindless  (cmd, VK_SHADER_STAGE_COMPUTE_BIT, pipeline_st.layout);
	G_CmdBindPipeline  (cmd, pipeline_st.bind_point, pipeline_st.pipeline);
	G_CmdPushConstants (cmd, pipeline_st.layout, VK_SHADER_STAGE_COMPUTE_BIT, sizeof(args), &args, 0);

	G_CmdDispatch      (cmd,
						  G_ComputeGroupCount(output_texture->width,  8),
						  G_ComputeGroupCount(output_texture->height, 8),
						  1);
}
