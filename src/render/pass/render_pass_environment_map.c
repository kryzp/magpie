
internal R_PASS_RECORD_DEF(R_HdrToEnvPassFn)
{
	G_CmdBuffer *cmd = ctx->cmd;
	const R_HdrToEnvPassData *user_data = ctx->user_data;
	const R_FrameParams *frame_params = user_data->frame_params;

	G_GraphicsPipelineDef pipeline_def = G_GraphicsPipelineDefFromInfo(frame_params->hdr_to_cubemap_shader, ctx->render_info);
	
	G_PipelineSt pipeline_st = G_FetchGraphicsPipeline(&pipeline_def);
	
	struct
	{
		u64 transform_matrix_buffer;
		u64 vertex_buffer;
		u32 hdr_image;
		u32 linear_sampler;
	}
	args;
	
	args.transform_matrix_buffer = G_BufferAddress(frame_params->cubemap_capture_transform_buffer);
	args.vertex_buffer = G_BufferAddress(frame_params->skybox_mesh->vertex_buffer);
	args.hdr_image = G_TextureViewBindless(user_data->hdr_view);
	args.linear_sampler = G_SamplerBindless(frame_params->linear_sampler);

	G_CmdBindBindless(cmd, VK_SHADER_STAGE_ALL_GRAPHICS, pipeline_st.layout);
	G_CmdBindPipeline(cmd, pipeline_st.bind_point, pipeline_st.pipeline);	
	G_CmdPushConstants(cmd, pipeline_st.layout, VK_SHADER_STAGE_ALL_GRAPHICS, args, 0);

	R_MeshBindIndexBuffer(frame_params->skybox_mesh, cmd);
	R_MeshDraw(frame_params->skybox_mesh, cmd);
}
