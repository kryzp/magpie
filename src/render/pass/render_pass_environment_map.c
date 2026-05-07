
R_PASS_RECORD_DEF(R_HdrToEnvPassFn)
{
	GFX_Device *device = ctx->device;
	GFX_CmdBuffer *cmd = ctx->cmd;
	
	const R_HdrToEnvPassData *user_data = ctx->user_data;
	
	GFX_GraphicsPipelineDef pipeline_def = GFX_GraphicsPipelineDefFromInfo(user_data->shader, ctx->render_info);
	
	GFX_PipelineSt pipeline_st = GFX_DeviceFetchGraphicsPipeline(device, &pipeline_def);
	
	struct
	{
		u64 transform_matrix_buffer;
		u64 vertex_buffer;
		u32 hdr_image;
		u32 linear_sampler;
	}
	args;
	
	args.transform_matrix_buffer = GFX_DeviceBufferAddress       (device, user_data->capture_transforms);
	args.vertex_buffer           = GFX_DeviceBufferAddress       (device, user_data->skybox_mesh->vertex_buffer);
	args.hdr_image               = GFX_DeviceTextureViewBindless (device, user_data->hdr_view);
	args.linear_sampler          = GFX_DeviceSamplerBindless     (device, user_data->sampler);

	GFX_CmdBindBindless  (cmd, VK_SHADER_STAGE_ALL_GRAPHICS, pipeline_st.layout);
	GFX_CmdBindPipeline  (cmd, pipeline_st.bind_point, pipeline_st.pipeline);	
	GFX_CmdPushConstants (cmd, pipeline_st.layout, VK_SHADER_STAGE_ALL_GRAPHICS, sizeof(args), &args, 0);

	R_MeshBind(user_data->skybox_mesh, cmd);
	R_MeshDraw(user_data->skybox_mesh, cmd);
}
