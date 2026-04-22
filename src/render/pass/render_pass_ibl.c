
/*
typedef struct R_PassContext R_PassContext;
struct R_PassContext
{
	R_Graph *graph;
	
	GFX_Device *device;	
	GFX_CmdBuffer *cmd;

	const R_Scene *scene;
	const R_Camera *camera;
	
	f32 delta_time;
	f32 elapsed_time;

	const void *user_data;
};
*/

// what are you gonna do about it
typedef struct R_IBLPassRenderIrradianceRecordUserData R_IBLPassRenderIrradianceRecordUserData;
struct R_IBLPassRenderIrradianceRecordUserData
{
	GFX_ShaderKey shader;
	GFX_SamplerKey sampler;
	GFX_TextureViewKey env_view;
	const R_Mesh *cube_mesh;
	GFX_BufferKey capture_transforms;
};

R_PASS_RECORD_DEF(R_IBLPassRenderIrradianceRecord)
{
	GFX_Device *device = ctx->device;
	GFX_CmdBuffer *cmd = ctx->cmd;
	const R_IBLPassRenderIrradianceRecordUserData *user_data = ctx->user_data;
	
	GFX_GraphicsPipelineDef pipeline_def = GFX_GraphicsPipelineDefInit(user_data->shader);
	pipeline_def.multi_view_mask = 0b111111;
	pipeline_def.colour_attachment_count = 1;
	pipeline_def.colour_attachment_formats[0] = VK_FORMAT_R32G32B32A32_SFLOAT;
	
	GFX_PipelineSt pipeline_st = GFX_DeviceFetchGraphicsPipeline(device, &pipeline_def);
	
	GFX_CmdBindBindless(cmd, VK_SHADER_STAGE_ALL_GRAPHICS, pipeline_st.layout, &cmd->device->bindless);
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
	args.vertex_buffer           = GFX_DeviceBufferAddress       (device, user_data->cube_mesh->vertex_buffer);
	args.environment_map         = GFX_DeviceTextureViewBindless (device, user_data->env_view);
	args.linear_sampler          = GFX_DeviceSamplerBindless     (device, user_data->sampler);
	
	GFX_CmdPushConstants   (cmd, pipeline_st.layout, VK_SHADER_STAGE_ALL_GRAPHICS, sizeof(args), &args, 0);
	GFX_CmdBindIndexBuffer (cmd, user_data->cube_mesh->index_buffer, 0, VK_WHOLE_SIZE);
	GFX_CmdDrawIndexed     (cmd, user_data->cube_mesh->index_count, 1, 0, 0, 0);
}

internal void
R_IBLPassRenderIrradiance(R_Graph *graph,
						  Arena *frame_arena,
						  const GFX_Device *device,
						  GFX_TextureKey out,
						  GFX_TextureKey environment_map,
						  const R_Mesh *skybox,
						  GFX_BufferKey capture_transforms)
{
	R_Pass *pass = R_GraphAdd(graph, String8Lit("Irradiance"), R_PassType_Graphics);
	
	R_GraphTexHandle out_handle = R_GraphImportTexture(graph, device, out);
	
	R_IBLPassRenderIrradianceRecordUserData *user_data = ArenaPushArray(frame_arena, R_IBLPassRenderIrradianceRecordUserData, 1);
	user_data->shader = GFX_ShaderKeyNull();
	user_data->sampler = GFX_SamplerKeyNull();
	user_data->env_view = GFX_TextureViewKeyNull();
	user_data->cube_mesh = skybox;
	user_data->capture_transforms = capture_transforms;
	
	R_PassSetRecord           (pass, R_IBLPassRenderIrradianceRecord, user_data);
	R_PassSetMultiViewMask    (pass, 0b111111);
	R_PassWriteColour         (pass, out_handle, NULL);
}

R_PASS_RECORD_DEF(R_IBLPassRenderPrefilterRecord)
{
}

internal void
R_IBLPassRenderPrefilter(R_Graph *graph,
						 Arena *frame_arena,
						 const GFX_Device *device,
						 GFX_TextureKey out,
						 GFX_TextureKey environment_map,
						 const R_Mesh *skybox,
						 GFX_BufferKey capture_transforms)
{
	AssertTrue(false && "fuck you work in progress");
}
