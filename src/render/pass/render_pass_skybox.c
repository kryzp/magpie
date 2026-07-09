
static R_PASS_RECORD_DEF(R_SkyboxPassFn)
{
	G_CmdBuffer *cmd = ctx->cmd;
	
	const R_SkyboxPassData *user_data = ctx->user_data;

	G_GraphicsPipelineDef pipeline_def = G_GraphicsPipelineDefFromInfo(user_data->shader, ctx->render_info);
	pipeline_def.depth_stencil_state.depth_test_enabled = true;
	pipeline_def.depth_stencil_state.depth_write_enabled = false;
	pipeline_def.depth_stencil_state.depth_compare_op = VK_COMPARE_OP_LESS_OR_EQUAL;

	G_PipelineSt pipeline_st = G_DeviceFetchGraphicsPipeline(&pipeline_def);
	
	struct
	{
		u64 frame_data_buffer;
		u64 vertex_buffer;
		u32 cubemap_texture;
		u32 linear_sampler;
	}
	args;
	
	args.frame_data_buffer = user_data->frame_params->frame_data.gpu;
	args.vertex_buffer = G_DeviceBufferAddress(user_data->skybox_mesh->vertex_buffer);
	args.cubemap_texture = G_DeviceTextureViewBindless(user_data->cubemap);
	args.linear_sampler = G_DeviceSamplerBindless(user_data->frame_params->linear_sampler);

	G_CmdBindBindless(cmd, VK_SHADER_STAGE_ALL_GRAPHICS, pipeline_st.layout);
	G_CmdBindPipeline(cmd, pipeline_st.bind_point, pipeline_st.pipeline);	
	G_CmdPushConstants(cmd, pipeline_st.layout, VK_SHADER_STAGE_ALL_GRAPHICS, args, 0);

	R_MeshBindIndexBuffer(user_data->skybox_mesh, cmd);
	R_MeshDraw(user_data->skybox_mesh, cmd);
}
