
static R_PASS_RECORD_DEF(R_HdrToEnvPassFn)
{
	G_CmdBuffer *cmd = ctx->cmd;
	
	const R_HdrToEnvPassData *user_data = ctx->user_data;
	
	G_GraphicsPipelineDef pipeline_def = G_GraphicsPipelineDefFromInfo(user_data->shader, ctx->render_info);
	
	G_PipelineSt pipeline_st = G_DeviceFetchGraphicsPipeline(&pipeline_def);
	
	struct
	{
		u64 transform_matrix_buffer;
		u64 vertex_buffer;
		u32 hdr_image;
		u32 linear_sampler;
	}
	args;
	
	args.transform_matrix_buffer = G_DeviceBufferAddress(user_data->capture_transforms);
	args.vertex_buffer = G_DeviceBufferAddress(user_data->skybox_mesh->vertex_buffer);
	args.hdr_image = G_DeviceTextureViewBindless(user_data->hdr_view);
	args.linear_sampler = G_DeviceSamplerBindless(user_data->sampler);

	G_CmdBindBindless(cmd, VK_SHADER_STAGE_ALL_GRAPHICS, pipeline_st.layout);
	G_CmdBindPipeline(cmd, pipeline_st.bind_point, pipeline_st.pipeline);	
	G_CmdPushConstants(cmd, pipeline_st.layout, VK_SHADER_STAGE_ALL_GRAPHICS, sizeof(args), &args, 0);

	R_MeshBind(user_data->skybox_mesh, cmd);
	R_MeshDraw(user_data->skybox_mesh, cmd);
}
