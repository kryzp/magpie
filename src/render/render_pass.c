
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

	AssertTrue(pass->input_texture_count < ArraySize(pass->input_textures));

	pass->input_textures[pass->input_texture_count] = edge;
	pass->input_texture_count++;

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

	AssertTrue(pass->output_texture_count < ArraySize(pass->output_textures));

	pass->output_textures[pass->output_texture_count] = edge;
	pass->output_texture_count++;

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

	AssertTrue(pass->input_buffer_count < ArraySize(pass->input_buffers));

	pass->input_buffers[pass->input_buffer_count] = edge;
	pass->input_buffer_count++;

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

	AssertTrue(pass->output_buffer_count < ArraySize(pass->output_buffers));

	pass->output_buffers[pass->output_buffer_count] = edge;
	pass->output_buffer_count++;

	return versioned_handle;
}

internal R_GraphTexHandle
R_PassWriteColour(R_Pass *pass, R_GraphTexHandle handle, const R_Clear *clear)
{
	return R_PassWriteColourEx(pass, handle, clear, G_SubresourceRangeAllColour());
}

internal R_GraphTexHandle
R_PassWriteDepth(R_Pass *pass, R_GraphTexHandle handle, const R_Clear *clear)
{
	return R_PassWriteDepthEx(pass, handle, clear, G_SubresourceRangeAllDepth());
}

internal R_GraphTexHandle
R_PassWriteColourEx(R_Pass *pass, R_GraphTexHandle handle, const R_Clear *clear, G_SubresourceRange range)
{
	R_GraphTexHandle versioned_handle = R_GraphPushTexVersion(pass->graph, handle, pass->index);
	
	R_PassTextureEdge edge = {0};
	
	edge.handle = versioned_handle;
	edge.state.stage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
	edge.state.access = VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
	edge.layout = VK_IMAGE_LAYOUT_GENERAL;
	edge.attachment_range = range;

	edge.should_clear = clear != NULL;

	if (edge.should_clear)
		edge.clear = *clear;

	AssertTrue(pass->output_texture_count < ArraySize(pass->output_textures));

	pass->output_textures[pass->output_texture_count] = edge;
	pass->output_texture_count++;

	return versioned_handle;
}

internal R_GraphTexHandle
R_PassWriteDepthEx(R_Pass *pass, R_GraphTexHandle handle, const R_Clear *clear, G_SubresourceRange range)
{
	R_GraphTexHandle versioned_handle = R_GraphPushTexVersion(pass->graph, handle, pass->index);
	
	R_PassTextureEdge edge = {0};
	
	edge.handle = versioned_handle;
	edge.state.stage = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
	edge.state.access = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	edge.layout = VK_IMAGE_LAYOUT_GENERAL;
	edge.attachment_range = range;

	edge.should_clear = clear != NULL;

	if (edge.should_clear)
		edge.clear = *clear;

	AssertTrue(pass->output_texture_count < ArraySize(pass->output_textures));

	pass->output_textures[pass->output_texture_count] = edge;
	pass->output_texture_count++;

	return versioned_handle;
}

internal R_GraphTexHandle
R_PassWriteColourResolve(R_Pass *pass, R_GraphTexHandle msaa, R_GraphTexHandle resolve, const R_Clear *clear)
{
	return R_PassWriteColourResolveEx(pass, msaa, resolve, clear, G_SubresourceRangeAllColour());
}

internal R_GraphTexHandle
R_PassWriteDepthResolve(R_Pass *pass, R_GraphTexHandle msaa, R_GraphTexHandle resolve, const R_Clear *clear)
{
	return R_PassWriteDepthResolveEx(pass, msaa, resolve, clear, G_SubresourceRangeAllDepth());
}

internal R_GraphTexHandle
R_PassWriteColourResolveEx(R_Pass *pass, R_GraphTexHandle msaa, R_GraphTexHandle resolve, const R_Clear *clear, G_SubresourceRange range)
{
	R_GraphTexHandle msaa_v    = R_GraphPushTexVersion(pass->graph, msaa,    pass->index);
	R_GraphTexHandle resolve_v = R_GraphPushTexVersion(pass->graph, resolve, pass->index);
	
	R_PassTextureEdge edge = {0};
	
	edge.handle = msaa_v;
	edge.state.stage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
	edge.state.access = VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
	edge.layout = VK_IMAGE_LAYOUT_GENERAL;
	edge.attachment_range = range;

	edge.resolve_handle = resolve_v;
	edge.resolve_mode = VK_RESOLVE_MODE_AVERAGE_BIT;
	edge.resolve_layout = VK_IMAGE_LAYOUT_GENERAL;

	edge.should_clear = clear != NULL;

	if (edge.should_clear)
		edge.clear = *clear;

	AssertTrue(pass->output_texture_count < ArraySize(pass->output_textures));

	pass->output_textures[pass->output_texture_count] = edge;
	pass->output_texture_count++;

	return resolve_v; // downstream passes get the resolved version
}

internal R_GraphTexHandle
R_PassWriteDepthResolveEx(R_Pass *pass, R_GraphTexHandle msaa, R_GraphTexHandle resolve, const R_Clear *clear, G_SubresourceRange range)
{
	R_GraphTexHandle msaa_v    = R_GraphPushTexVersion(pass->graph, msaa,    pass->index);
	R_GraphTexHandle resolve_v = R_GraphPushTexVersion(pass->graph, resolve, pass->index);
	
	R_PassTextureEdge edge = {0};
	
	edge.handle = msaa_v;
	edge.state.stage = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
	edge.state.access = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	edge.layout = VK_IMAGE_LAYOUT_GENERAL;
	edge.attachment_range = range;

	edge.resolve_handle = resolve_v;
	edge.resolve_mode = VK_RESOLVE_MODE_MIN_BIT;
	edge.resolve_layout = VK_IMAGE_LAYOUT_GENERAL;

	edge.should_clear = clear != NULL;

	if (edge.should_clear)
		edge.clear = *clear;
	
	AssertTrue(pass->output_texture_count < ArraySize(pass->output_textures));

	pass->output_textures[pass->output_texture_count] = edge;
	pass->output_texture_count++;

	return resolve_v; // downstream passes get the resolved version
}

internal R_GraphTexHandle
R_PassReadTextureGraphics(R_Pass *pass, R_GraphTexHandle handle)
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
