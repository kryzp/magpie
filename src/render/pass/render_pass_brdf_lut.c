
static R_PASS_RECORD_DEF(R_BRDFLutPassFn)
{
	G_Device *device = ctx->device;
	G_CmdBuffer *cmd = ctx->cmd;
	
	const R_BRDFLutPassData *user_data = ctx->user_data;
	
	G_GraphicsPipelineDef pipeline_def = G_GraphicsPipelineDefInit(user_data->shader);
	pipeline_def.colour_attachment_count = 1;
	pipeline_def.colour_attachment_formats[0] = VK_FORMAT_R32G32_SFLOAT;

	G_PipelineSt pipeline_st = G_DeviceFetchGraphicsPipeline(device, &pipeline_def);

	G_CmdBindBindless (cmd, VK_SHADER_STAGE_ALL_GRAPHICS, pipeline_st.layout);
	G_CmdBindPipeline (cmd, pipeline_st.bind_point, pipeline_st.pipeline);
	G_CmdDrawV        (cmd, 3);
}
