
R_PASS_RECORD_DEF(R_SkyboxPassFn)
{
	GFX_Device *device = ctx->device;
	GFX_CmdBuffer *cmd = ctx->cmd;
	
	const R_SkyboxPassData *user_data = ctx->user_data;
	
	GFX_GraphicsPipelineDef pipeline_def = GFX_GraphicsPipelineDefInit(user_data->shader);
	pipeline_def.colour_attachment_count = 1;
	pipeline_def.colour_attachment_formats[0] = VK_FORMAT_R32G32B32A32_SFLOAT;

	/*
	pipeline_def.has_depth_attachment = true;
	pipeline_def.depth_stencil_state.depth_test_enabled = true;
	pipeline_def.depth_stencil_state.depth_write_enabled = false;
	pipeline_def.depth_stencil_state.depth_compare_op = VK_COMPARE_OP_LESS_OR_EQUAL;
	*/
	
	GFX_PipelineSt pipeline_st = GFX_DeviceFetchGraphicsPipeline(device, &pipeline_def);
	
	struct
	{
		u64 frame_data_buffer;
		u64 vertex_buffer;
		u32 cubemap_texture;
		u32 linear_sampler;
	}
	args;
	
	args.frame_data_buffer = GFX_DeviceBufferAddress       (device, user_data->frame_data_buffer);
	args.vertex_buffer     = GFX_DeviceBufferAddress       (device, user_data->skybox_mesh->vertex_buffer);
	args.cubemap_texture   = GFX_DeviceTextureViewBindless (device, user_data->cubemap);
	args.linear_sampler    = GFX_DeviceSamplerBindless     (device, user_data->sampler);

	GFX_CmdBindBindless  (cmd, VK_SHADER_STAGE_ALL_GRAPHICS, pipeline_st.layout);
	GFX_CmdBindPipeline  (cmd, pipeline_st.bind_point, pipeline_st.pipeline);	
	GFX_CmdPushConstants (cmd, pipeline_st.layout, VK_SHADER_STAGE_ALL_GRAPHICS, sizeof(args), &args, 0);

	R_MeshBind(user_data->skybox_mesh, cmd);
	R_MeshDraw(user_data->skybox_mesh, cmd);
}
