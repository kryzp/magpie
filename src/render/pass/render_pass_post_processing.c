
internal R_PASS_RECORD_DEF(R_PostProcessingPassFn)
{
	G_CmdBuffer *cmd = ctx->cmd;
	const R_PostProcessingPassData *user_data = ctx->user_data;
	const R_FrameParams *frame_params = user_data->frame_params;
	
	G_ComputePipelineDef pipeline_def = G_ComputePipelineDefInit(frame_params->tonemapping_shader);

	G_PipelineSt pipeline_st = G_FetchComputePipeline(&pipeline_def);
	
	struct
	{
		u32 width;
		u32 height;
		f32 exposure;
		u32 input_texture;
		u32 output_texture;
	}
	args;

	G_ResourceKey input_key = R_GraphResolveTexture(ctx->graph, user_data->input);
	G_ResourceKey output_key = R_GraphResolveTexture(ctx->graph, user_data->output);
	
	G_Texture *input_texture = G_TextureFromKey(input_key);
	G_Texture *output_texture = G_TextureFromKey(output_key);

	G_ResourceKey input_view = G_TextureViewAuto(input_key);
	G_ResourceKey output_view = G_TextureViewAuto(output_key);
	
	args.width = input_texture->width;
	args.height = input_texture->height;
	args.exposure = user_data->exposure;
	args.input_texture  = G_TextureViewBindless(input_view);
	args.output_texture = G_TextureViewBindless(output_view);
	
	G_CmdBindBindless(cmd, VK_SHADER_STAGE_COMPUTE_BIT, pipeline_st.layout);
	G_CmdBindPipeline(cmd, pipeline_st.bind_point, pipeline_st.pipeline);
	G_CmdPushConstants(cmd, pipeline_st.layout, VK_SHADER_STAGE_COMPUTE_BIT, args, 0);

	G_CmdDispatch(cmd,
				  G_ComputeGroupCount(output_texture->width,  8),
				  G_ComputeGroupCount(output_texture->height, 8),
				  1);
}
