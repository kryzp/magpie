
R_PASS_RECORD_DEF(R_BRDFLutPassFn)
{
	GFX_Device *device = ctx->device;
	GFX_CmdBuffer *cmd = ctx->cmd;
	
	const R_BRDFLutPassData *user_data = ctx->user_data;
	
	GFX_GraphicsPipelineDef pipeline_def = GFX_GraphicsPipelineDefInit(user_data->shader);
	pipeline_def.colour_attachment_count = 1;
	pipeline_def.colour_attachment_formats[0] = VK_FORMAT_R32G32_SFLOAT;

	GFX_PipelineSt pipeline_st = GFX_DeviceFetchGraphicsPipeline(device, &pipeline_def);

	GFX_CmdBindBindless (cmd, VK_SHADER_STAGE_ALL_GRAPHICS, pipeline_st.layout);
	GFX_CmdBindPipeline (cmd, pipeline_st.bind_point, pipeline_st.pipeline);
	GFX_CmdDrawV        (cmd, 3);
}
