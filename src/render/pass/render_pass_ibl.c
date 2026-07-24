
internal R_PASS_RECORD_DEF(R_BRDFLutPassFn)
{
	G_CmdBuffer *cmd = ctx->cmd;
	const R_BRDFLutPassData *user_data = ctx->user_data;
	const R_FrameParams *frame_params = user_data->frame_params;
	
	G_GraphicsPipelineDef pipeline_def = G_GraphicsPipelineDefFromInfo(frame_params->brdf_lut_generation_shader, ctx->render_info);

	G_PipelineSt pipeline_st = G_FetchGraphicsPipeline(&pipeline_def);

	G_CmdBindBindless(cmd, VK_SHADER_STAGE_ALL_GRAPHICS, pipeline_st.layout);
	G_CmdBindPipeline(cmd, pipeline_st.bind_point, pipeline_st.pipeline);
	G_CmdDrawV(cmd, 3);
}

internal R_PASS_RECORD_DEF(R_IBLPassIrradianceFn)
{
	G_CmdBuffer *cmd = ctx->cmd;
	const R_IBLPassIrradianceData *user_data = ctx->user_data;
	const R_FrameParams *frame_params = user_data->frame_params;

	G_GraphicsPipelineDef pipeline_def = G_GraphicsPipelineDefFromInfo(frame_params->irradiance_cubemap_gen_shader, ctx->render_info);
	
	G_PipelineSt pipeline_st = G_FetchGraphicsPipeline(&pipeline_def);
	
	G_CmdBindBindless(cmd, VK_SHADER_STAGE_ALL_GRAPHICS, pipeline_st.layout);
	G_CmdBindPipeline(cmd, pipeline_st.bind_point, pipeline_st.pipeline);
	
	struct
	{
		u64 transform_matrix_buffer;
		u64 vertex_buffer;
		u32 environment_map;
		u32 linear_sampler;
	}
	args;
	
	args.transform_matrix_buffer = G_BufferAddress(frame_params->cubemap_capture_transform_buffer);
	args.vertex_buffer           = G_BufferAddress(frame_params->skybox_mesh->vertex_buffer);
	args.environment_map         = G_TextureViewBindless(user_data->env_view);
	args.linear_sampler          = G_SamplerBindless(frame_params->linear_sampler);

	G_CmdPushConstants(cmd, pipeline_st.layout, VK_SHADER_STAGE_ALL_GRAPHICS, args, 0);

	R_MeshBindIndexBuffer(frame_params->skybox_mesh, cmd);
	R_MeshDraw(frame_params->skybox_mesh, cmd);

}

internal R_PASS_RECORD_DEF(R_IBLPassPrefilterFn)
{
	G_CmdBuffer *cmd = ctx->cmd;
	const R_IBLPassPrefilterData *user_data = ctx->user_data;
	const R_FrameParams *frame_params = user_data->frame_params;
	
	G_GraphicsPipelineDef pipeline_def = G_GraphicsPipelineDefInit(frame_params->prefilter_cubemap_gen_shader);
	pipeline_def.multi_view_mask = 0b111111;
	pipeline_def.colour_attachment_count = 1;
	pipeline_def.colour_attachment_formats[0] = VK_FORMAT_R32G32B32A32_SFLOAT;
	
	G_PipelineSt pipeline_st = G_FetchGraphicsPipeline(&pipeline_def);
	
	G_CmdBindBindless(cmd, VK_SHADER_STAGE_ALL_GRAPHICS, pipeline_st.layout);
	G_CmdBindPipeline(cmd, pipeline_st.bind_point, pipeline_st.pipeline);
	
	struct
	{
		u64 transform_matrix_buffer;
		u64 vertex_buffer;
		u32 environment_map;
		u32 linear_sampler;
		f32 roughness;
	}
	args;
	
	args.transform_matrix_buffer = G_BufferAddress(frame_params->cubemap_capture_transform_buffer);
	args.vertex_buffer           = G_BufferAddress(frame_params->skybox_mesh->vertex_buffer);
	args.environment_map         = G_TextureViewBindless(user_data->env_view);
	args.linear_sampler          = G_SamplerBindless(frame_params->linear_sampler);
	args.roughness               = user_data->roughness;

	G_CmdPushConstants(cmd, pipeline_st.layout, VK_SHADER_STAGE_ALL_GRAPHICS, args, 0);

	R_MeshBindIndexBuffer(frame_params->skybox_mesh, cmd);
	R_MeshDraw(frame_params->skybox_mesh, cmd);
}
