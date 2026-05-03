
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

R_PASS_RECORD_DEF(R_IBLPassIrradianceFn)
{
	GFX_Device *device = ctx->device;
	GFX_CmdBuffer *cmd = ctx->cmd;
	
	const R_IBLPassIrradianceData *user_data = ctx->user_data;
	
	GFX_GraphicsPipelineDef pipeline_def = GFX_GraphicsPipelineDefInit(user_data->shader);
	pipeline_def.multi_view_mask = 0b111111;
	pipeline_def.colour_attachment_count = 1;
	pipeline_def.colour_attachment_formats[0] = VK_FORMAT_R32G32B32A32_SFLOAT;
	
	GFX_PipelineSt pipeline_st = GFX_DeviceFetchGraphicsPipeline(device, &pipeline_def);
	
	GFX_CmdBindBindless(cmd, VK_SHADER_STAGE_ALL_GRAPHICS, pipeline_st.layout);
	GFX_CmdBindPipeline(cmd, pipeline_st.bind_point, pipeline_st.pipeline);
	
	struct
	{
		u64 transform_matrix_buffer;
		u64 vertex_buffer;
		u32 environment_map;
		u32 linear_sampler;
	}
	args;
	
	args.transform_matrix_buffer = GFX_DeviceBufferAddress       (device, user_data->capture_transforms);
	args.vertex_buffer           = GFX_DeviceBufferAddress       (device, user_data->skybox_mesh->vertex_buffer);
	args.environment_map         = GFX_DeviceTextureViewBindless (device, user_data->env_view);
	args.linear_sampler          = GFX_DeviceSamplerBindless     (device, user_data->sampler);

	GFX_CmdPushConstants(cmd, pipeline_st.layout, VK_SHADER_STAGE_ALL_GRAPHICS, sizeof(args), &args, 0);

	R_MeshBind(user_data->skybox_mesh, cmd);
	R_MeshDraw(user_data->skybox_mesh, cmd);

}

R_PASS_RECORD_DEF(R_IBLPassPrefilterFn)
{
	GFX_Device *device = ctx->device;
	GFX_CmdBuffer *cmd = ctx->cmd;
	
	const R_IBLPassPrefilterData *user_data = ctx->user_data;
	
	GFX_GraphicsPipelineDef pipeline_def = GFX_GraphicsPipelineDefInit(user_data->shader);
	pipeline_def.multi_view_mask = 0b111111;
	pipeline_def.colour_attachment_count = 1;
	pipeline_def.colour_attachment_formats[0] = VK_FORMAT_R32G32B32A32_SFLOAT;
	
	GFX_PipelineSt pipeline_st = GFX_DeviceFetchGraphicsPipeline(device, &pipeline_def);
	
	GFX_CmdBindBindless(cmd, VK_SHADER_STAGE_ALL_GRAPHICS, pipeline_st.layout);
	GFX_CmdBindPipeline(cmd, pipeline_st.bind_point, pipeline_st.pipeline);
	
	struct
	{
		u64 transform_matrix_buffer;
		u64 vertex_buffer;
		u32 environment_map;
		u32 linear_sampler;
		f32 roughness;
	}
	args;
	
	args.transform_matrix_buffer = GFX_DeviceBufferAddress       (device, user_data->capture_transforms);
	args.vertex_buffer           = GFX_DeviceBufferAddress       (device, user_data->skybox_mesh->vertex_buffer);
	args.environment_map         = GFX_DeviceTextureViewBindless (device, user_data->env_view);
	args.linear_sampler          = GFX_DeviceSamplerBindless     (device, user_data->sampler);
	args.roughness               = user_data->roughness;

	GFX_CmdPushConstants(cmd, pipeline_st.layout, VK_SHADER_STAGE_ALL_GRAPHICS, sizeof(args), &args, 0);

	R_MeshBind(user_data->skybox_mesh, cmd);
	R_MeshDraw(user_data->skybox_mesh, cmd);
}
