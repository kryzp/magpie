
internal void
R_PassSetRecord(R_Pass *pass, R_PassRecordFn *fn, const void *user_data)
{
	pass->record = fn;
	pass->user_data = user_data;
}

internal void
R_PassSetMultiViewMask(R_Pass *pass, u32 mask)
{
	pass->multi_view_mask = mask;
}

internal R_GraphTexHandle
R_PassAddInputTexture(R_Pass *pass,
					  R_GraphTexHandle handle,
					  VkPipelineStageFlags2 stage,
					  VkAccessFlags2 access)
{
	R_GraphTexHandle versioned_handle = handle;
	
	R_PassTextureEdge edge = {0};
	edge.handle = versioned_handle;
	edge.state.stage = stage;
	edge.state.access = access;
	edge.layout = VK_IMAGE_LAYOUT_GENERAL;

	edge.should_clear = false;

	pass->input_textures[pass->input_texture_count] = edge;
	pass->input_texture_count++;

	AssertTrue(pass->input_texture_count < ArraySize(pass->input_textures));

	return versioned_handle;
}

internal R_GraphTexHandle
R_PassAddOutputTexture(R_Pass *pass,
					   R_GraphTexHandle handle,
					   const R_Clear *clear,
					   VkPipelineStageFlags2 stage,
					   VkAccessFlags2 access)
{
	R_GraphTexHandle versioned_handle = R_GraphPushTexVersion(pass->graph, handle, pass->index);
	
	R_PassTextureEdge edge = {0};
	edge.handle = versioned_handle;
	edge.state.stage = stage;
	edge.state.access = access;
	edge.layout = VK_IMAGE_LAYOUT_GENERAL;

	edge.should_clear = clear != NULL;

	if (edge.should_clear)
		edge.clear = *clear;

	pass->input_textures[pass->input_texture_count] = edge;
	pass->input_texture_count++;

	AssertTrue(pass->input_texture_count < ArraySize(pass->input_textures));

	return versioned_handle;
}

internal R_GraphBufHandle
R_PassAddInputBuffer(R_Pass *pass,
					 R_GraphBufHandle handle,
					 VkPipelineStageFlags2 stage,
					 VkAccessFlags2 access)
{
	R_GraphBufHandle versioned_handle = handle;
	
	R_PassBufferEdge edge = {0};
	edge.handle = versioned_handle;
	edge.state.stage = stage;
	edge.state.access = access;
	edge.offset = 0;
	edge.size = 0;

	pass->input_buffers[pass->input_buffer_count] = edge;
	pass->input_buffer_count++;

	AssertTrue(pass->input_buffer_count < ArraySize(pass->input_buffers));

	return versioned_handle;
}

internal R_GraphBufHandle
R_PassAddOutputBuffer(R_Pass *pass,
					  R_GraphBufHandle handle,
					  VkPipelineStageFlags2 stage,
					  VkAccessFlags2 access)
{
	R_GraphBufHandle versioned_handle = R_GraphPushBufVersion(pass->graph, handle, pass->index);
	
	R_PassBufferEdge edge = {0};
	edge.handle = versioned_handle;
	edge.state.stage = stage;
	edge.state.access = access;
	edge.offset = 0;
	edge.size = 0;

	pass->output_buffers[pass->output_buffer_count] = edge;
	pass->output_buffer_count++;

	AssertTrue(pass->output_buffer_count < ArraySize(pass->output_buffers));

	return versioned_handle;
}

internal R_GraphTexHandle
R_PassWriteColour(R_Pass *pass, R_GraphTexHandle handle, const R_Clear *clear)
{
	return R_PassAddOutputTexture(pass, handle, clear,
								  VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
								  VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT);
}

internal R_GraphTexHandle
R_PassWriteDepth(R_Pass *pass, R_GraphTexHandle handle, const R_Clear *clear)
{
	return R_PassAddOutputTexture(pass, handle, clear,
								  VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
								  VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);
}

internal R_GraphTexHandle
R_PassReadTexture(R_Pass *pass, R_GraphTexHandle handle)
{
	return R_PassAddInputTexture(pass, handle,
								 VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
								 VK_ACCESS_2_SHADER_READ_BIT);
}

internal R_GraphTexHandle
R_PassReadTextureCompute(R_Pass *pass, R_GraphTexHandle handle)
{
	return R_PassAddInputTexture(pass, handle,
								 VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
								 VK_ACCESS_2_SHADER_READ_BIT);
}

internal R_GraphTexHandle
R_PassWriteTextureCompute(R_Pass *pass, R_GraphTexHandle handle)
{
	return R_PassAddOutputTexture(pass, handle, NULL,
								  VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
								  VK_ACCESS_2_SHADER_WRITE_BIT);
}

internal R_GraphTexHandle
R_PassBlitTextureSrc(R_Pass *pass, R_GraphTexHandle handle)
{
	return R_PassAddInputTexture(pass, handle,
								 VK_PIPELINE_STAGE_2_BLIT_BIT,
								 VK_ACCESS_2_TRANSFER_READ_BIT);
}

internal R_GraphTexHandle
R_PassBlitTextureDst(R_Pass *pass, R_GraphTexHandle handle)
{
	return R_PassAddOutputTexture(pass, handle, NULL,
								  VK_PIPELINE_STAGE_2_BLIT_BIT,
								  VK_ACCESS_2_TRANSFER_WRITE_BIT);
}

internal R_GraphBufHandle
R_PassWriteBufferGraphics(R_Pass *pass, R_GraphBufHandle handle)
{
	return R_PassAddOutputBuffer(pass, handle,
								 VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
								 VK_ACCESS_2_SHADER_WRITE_BIT);
}

internal R_GraphBufHandle
R_PassReadBufferGraphics(R_Pass *pass, R_GraphBufHandle handle)
{
	return R_PassAddInputBuffer(pass, handle,
								VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
								VK_ACCESS_2_SHADER_READ_BIT);
}

internal R_GraphBufHandle
R_PassWriteBufferCompute(R_Pass *pass, R_GraphBufHandle handle)
{
	return R_PassAddOutputBuffer(pass, handle,
								 VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
								 VK_ACCESS_2_SHADER_WRITE_BIT);
}

internal R_GraphBufHandle
R_PassReadBufferCompute(R_Pass *pass, R_GraphBufHandle handle)
{
	return R_PassAddInputBuffer(pass, handle,
								VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
								VK_ACCESS_2_SHADER_READ_BIT);
}

internal R_GraphBufHandle
R_PassIndirectBuffer(R_Pass *pass, R_GraphBufHandle handle)
{
	return R_PassAddInputBuffer(pass, handle,
								VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT,
								VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT);
}

internal R_GraphBufHandle
R_PassClearBuffer(R_Pass *pass, R_GraphBufHandle handle)
{
	return R_PassAddOutputBuffer(pass, handle,
								 VK_PIPELINE_STAGE_2_CLEAR_BIT,
								 VK_ACCESS_2_TRANSFER_WRITE_BIT);
}
