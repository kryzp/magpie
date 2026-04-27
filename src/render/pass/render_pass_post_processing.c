
R_PASS_RECORD_DEF(R_PostProcessingPassFn)
{
	GFX_CmdBuffer *cmd = ctx->cmd;
	
	const R_PostProcessingPassData *user_data = ctx->user_data;

	GFX_ComputePipelineDef pipeline_def = GFX_ComputePipelineDefInit(user_data->shader);

	GFX_PipelineSt pipeline_st = GFX_DeviceFetchComputePipeline(ctx->device, &pipeline_def);
	
	struct
	{
		u32 width;
		u32 height;
		f32 exposure;
		u32 input_texture;
		u32 output_texture;
	}
	args;

	GFX_TextureKey input_key  = R_GraphResolveTexture(ctx->graph, user_data->input);
	GFX_TextureKey output_key = R_GraphResolveTexture(ctx->graph, user_data->output);
	
	GFX_Texture *input_texture  = GFX_DeviceTextureFromKey(ctx->device, input_key);
	GFX_Texture *output_texture = GFX_DeviceTextureFromKey(ctx->device, output_key);

	GFX_TextureViewKey input_view  = GFX_DeviceTextureViewAuto(ctx->device, input_key);
	GFX_TextureViewKey output_view = GFX_DeviceTextureViewAuto(ctx->device, output_key);
	
	args.width          = input_texture->width;
	args.height         = input_texture->height;
	args.exposure       = user_data->exposure;
	args.input_texture  = GFX_DeviceTextureViewBindless(ctx->device, input_view);
	args.output_texture = GFX_DeviceTextureViewBindless(ctx->device, output_view);
	
	GFX_CmdBindBindless  (cmd, VK_SHADER_STAGE_COMPUTE_BIT, pipeline_st.layout);
	GFX_CmdBindPipeline  (cmd, pipeline_st.bind_point, pipeline_st.pipeline);
	GFX_CmdPushConstants (cmd, pipeline_st.layout, VK_SHADER_STAGE_COMPUTE_BIT, sizeof(args), &args, 0);

	GFX_CmdDispatch      (cmd,
						  GFX_ComputeGroupCount(output_texture->width,  8),
						  GFX_ComputeGroupCount(output_texture->height, 8),
						  1);
}
