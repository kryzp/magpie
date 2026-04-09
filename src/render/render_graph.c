
internal void
R_GraphInit(R_Graph *graph, Arena *permanent_arena, Arena *frame_arena)
{
	MemZeroStruct(graph);
	
	graph->permanent_arena = permanent_arena;
	graph->frame_arena = frame_arena;

	graph->backbuffer_handle = R_GraphTexHandleNull();
}

internal void
R_GraphDestroy(R_Graph *graph)
{
	R_ResourcePoolDestroy(&graph->pool);
}

internal void
R_GraphReset(R_Graph *graph, const GFX_Device *device)
{
	for (u32 i = 0; i < graph->texture_res_count; i++)
	{
		R_GraphTexture *t = &graph->texture_res[i];

		if (t->is_imported)
			R_ResourceTrackerSetTexture(&graph->tracker, t->imported_key, t->state);
		else
			R_ResourcePoolUpdateTexture(&graph->pool, t->physical_key, t->state);
	}
	
	for (u32 i = 0; i < graph->buffer_res_count; i++)
	{
		R_GraphBuffer *b = &graph->buffer_res[i];

		if (b->is_imported)
			R_ResourceTrackerSetBuffer(&graph->tracker, b->imported_key, b->state);
		else
			R_ResourcePoolUpdateBuffer(&graph->pool, b->physical_key, b->state);
	}
	
	graph->pass_count = 0;

	graph->texture_res_count = 0;
	graph->texture_ver_count = 0;

	graph->buffer_res_count = 0;
	graph->buffer_ver_count = 0;

	graph->imported_texture_count = 0;
	graph->imported_buffer_count = 0;

	R_ResourcePoolFlush(&graph->pool);

	graph->backbuffer_handle = R_GraphTexHandleNull();
}

internal R_Pass *
R_GraphAdd(R_Graph *graph, String8 name, R_PassType type)
{
	R_Pass *pass = &graph->passes[graph->pass_count];
	pass->graph = graph;
	pass->name = name;
	pass->type = type;
	pass->index = graph->pass_count;
	
	graph->pass_count++;

	AssertTrue(graph->pass_count < ArraySize(graph->passes));
	
	return pass;
}

internal void
R_GraphSetBackbuffer(R_Graph *graph, R_GraphTexHandle handle)
{
	graph->backbuffer_handle = handle;
}

internal R_GraphTexHandle
R_GraphCreateTexture(R_Graph *graph, const R_TextureInfo *info)
{
	R_GraphTexHandle handle = {0};
	handle.value = graph->texture_res_count;

	R_GraphTexture texture = {0};
	texture.texture_info = *info;
	texture.first_stage_index = -1u;
	texture.last_stage_index = -1u;
	texture.ref_count = 0;
	texture.is_imported = false;

	graph->texture_res[graph->texture_res_count] = texture;
	graph->texture_res_count++;

	return handle;
}

internal R_GraphBufHandle
R_GraphCreateBuffer(R_Graph *graph, const R_BufferInfo *info)
{
	R_GraphBufHandle handle = {0};
	handle.value = graph->buffer_res_count;

	R_GraphBuffer buffer = {0};
	buffer.buffer_info = *info;
	buffer.first_stage_index = -1u;
	buffer.last_stage_index = -1u;
	buffer.ref_count = 0;
	buffer.is_imported = false;

	graph->buffer_res[graph->buffer_res_count] = buffer;
	graph->buffer_res_count++;

	return handle;
}

internal R_GraphTexHandle
R_GraphImportTexture(R_Graph *graph, const GFX_Device *device, GFX_TextureKey external_key)
{
	// TODO
}

internal R_GraphBufHandle
R_GraphImportBuffer(R_Graph *graph, const GFX_Device *device, GFX_BufferKey external_key)
{
	// TODO
}

internal R_GraphTexHandle
R_GraphPushTexVersion(R_Graph *graph, R_GraphTexHandle parent, u32 write_pass_index)
{
	// TODO
}

internal R_GraphBufHandle
R_GraphPushBufVersion(R_Graph *graph, R_GraphBufHandle parent, u32 write_pass_index)
{
	// TODO
}

internal void
R_GraphCompile(R_Graph *graph, GFX_Device *device, const GFX_Swapchain *swapchain)
{
	for (u32 i = 0; i < texture_res_count; i++)
	{
		R_GraphTexture *t = &graph->texture_res[i];

		if (t->is_imported)
			t->ref_count = 1;
	}
	
	for (u32 i = 0; i < buffer_res_count; i++)
	{
		R_GraphBuffer *b = &graph->buffer_res[i];

		if (b->is_imported)
			b->ref_count = 1;
	}

	if (!R_GraphTexHandleIsNull(graph->backbuffer_handle))
		R_GraphTextureFromHandle(graph, graph->backbuffer_handle)->ref_count++;

	R_GraphPropogateDependencies(graph);
	R_GraphBackpropogateDependencies(graph);

	R_GraphAllocateResources(graph, device, swapchain);

	R_GraphGenerateBarriers(graph, device);
}

internal void
R_GraphPropogateDependencies(R_Graph *graph)
{
	for (u32 i = 0; i < graph->pass_count; i++)
	{
		R_Pass *pass = &graph->passes[i];

		for (u32 j = 0; j < pass->output_texture_count; j++)
		{
			R_PassTextureEdge *out = &pass->output_textures[j];
			
			if (R_GraphTextureFromHandle(graph, out->handle)->first_stage_index == -1u)
				R_GraphTextureFromHandle(graph, out->handle)first_stage_index = i;
		}

		for (u32 j = 0; j < pass->output_buffer_count; j++)
		{
			R_PassBufferEdge *out = &pass->output_buffers[j];
			
			if (R_GraphBufferFromHandle(graph, out->handle)->first_stage_index == -1u)
				R_GraphBufferFromHandle(graph, out->handle)->first_stage_index = i;
		}

		if (pass->is_culled)
			continue;

		for (u32 j = 0; j < pass->input_texture_count; j++)
		{
			R_PassTextureEdge *in = &pass->input_textures[j];
			
			if (R_GraphTextureFromHandle(graph, in->handle)->first_stage_index == -1u)
				R_GraphTextureFromHandle(graph, in->handle)->first_stage_index = i;
		}

		for (u32 j = 0; j < pass->input_buffer_count; j++)
		{
			R_PassBufferEdge *in = &pass->input_buffers[j];
			
			if (R_GraphBufferFromHandle(graph, in->handle)->first_stage_index == -1u)
				R_GraphBufferFromHandle(graph, in->handle)->first_stage_index = i;
		}
	}
}

internal void
R_GraphBackpropogateDependencies(R_Graph *graph)
{
	for (u32 i = graph->pass_count - 1; i >= 0; i--)
	{
		R_Pass *pass = &graph->passes[i];

		pass->is_culled = false;

		for (u32 j = 0; j < pass->output_texture_count; j++)
		{
			R_PassTextureEdge *out = &pass->output_textures[j];

			if (R_GraphTexHandleMatch(graph, out->handle, graph->backbuffer_handle))
				pass->is_culled = false;

			if (R_GraphTextureFromHandle(graph, out->handle)->ref_count > 0)
				pass->is_culled = false;
			
			if (R_GraphTextureFromHandle(graph, out->handle)->last_stage_index == -1u)
				R_GraphTextureFromHandle(graph, out->handle)->last_stage_index = i;
		}

		for (u32 j = 0; j < pass->output_buffer_count; j++)
		{
			R_PassBufferEdge *out = &pass->output_buffers[j];

			if (R_GraphBufferFromHandle(graph, out->handle)->ref_count > 0)
				pass->is_culled = false;
			
			if (R_GraphBufferFromHandle(graph, out->handle)->last_stage_index == -1u)
				R_GraphBufferFromHandle(graph, out->handle)->last_stage_index = i;
		}

		if (pass->is_culled)
			continue;

		for (u32 j = 0; j < pass->input_texture_count; j++)
		{
			R_PassTextureEdge *in = &pass->input_textures[j];

			R_GraphTextureFromHandle(graph, in->handle)->ref_count++;

			if (R_GraphTextureFromHandle(graph, in->handle)->last_stage_index == -1u)
				R_GraphTextureFromHandle(graph, in->handle)->last_stage_index = i;
		}

		for (u32 j = 0; j < pass->input_buffer_count; j++)
		{
			R_PassBufferEdge *in = &pass->input_buffers[j];

			R_GraphBufferFromHandle(graph, in->handle)->ref_count++;

			if (R_GraphBufferFromHandle(graph, in->handle)->last_stage_index == -1u)
				R_GraphBufferFromHandle(graph, in->handle)->last_stage_index = i;
		}
	}
}

internal void
R_GraphAllocateResources(R_Graph *graph, GFX_Device *device, const GFX_Swapchain *swapchain)
{
	for (u32 i = 0; i < graph->pass_count; i++)
	{
		R_Pass *pass = &graph->passes[i];

		if (pass->is_culled)
			continue;

		for (u32 j = 0; j < pass->output_texture_count; j++)
		{
			R_PassTextureEdge *out = &pass->output_textures[j];
			R_GraphTexture *t = R_GraphTextureFromHandle(graph, out->handle);

			if (t->is_imported)
				continue;

			if (!GFX_TextureKeyIsNull(t->physical_key))
				continue;

			// Resolve relative size.
			if (t->texture_info.size_class = R_SizeClass_SwapchainRelative)
			{
				t->texture_info.size_x *= swapchain->width;
				t->texture_info.size_y *= swapchain->height;

				t->texture_info.size_class = R_SizeClass_Absolute;
			}

			t->physical_key = R_ResourcePoolAcquireTexture(&graph->pool, device, &t->texture_info, &t->state);
		}

		for (u32 j = 0; j < pass->output_buffer_count; j++)
		{
			R_PassBufferEdge *out = &pass->output_buffers[j];
			R_GraphBuffer *b = R_GraphBufferFromHandle(graph, out->handle);

			if (b->is_imported)
				continue;

			if (!R_BufferKeyIsNull(b->physical_key))
				continue;

			t->physical_key = R_ResourcePoolAcquireBuffer(&graph->pool, device, &b->buffer_info, &b->state);
		}
	}
}

internal void
R_GraphGenerateBarriers(R_Graph *graph, const GFX_Device *device)
{
	for (u32 i = 0; i < graph->pass_count; i++)
	{
		R_Pass *pass = &graph->passes[i];

		if (pass->is_culled)
			continue;


		// -- Inputs
		
		for (u32 j = 0; j < pass->input_texture_count; j++)
			R_GraphProcessInvalidateTexture(graph, device, pass, &pass->input_textures[j]);

		for (u32 j = 0; j < pass->input_buffer_count; j++)
			R_GraphProcessInvalidateBuffer(graph, device, pass, &pass->input_buffers[j]);

		
		// -- Outputs
		
		for (u32 j = 0; j < pass->output_texture_count; j++)
			R_GraphProcessInvalidateTexture(graph, device, pass, &pass->output_textures[j]);

		for (u32 j = 0; j < pass->output_buffer_count; j++)
			R_GraphProcessInvalidateBuffer(graph, device, pass, &pass->output_buffers[j]);


		// -- Flush Outputs Also

		for (u32 j = 0; j < pass->output_texture_count; j++)
			R_GraphProcessFlushTexture(graph, device, pass, &pass->output_textures[j]);

		for (u32 j = 0; j < pass->output_buffer_count; j++)
			R_GraphProcessFlushBuffer(graph, device, pass, &pass->output_buffers[j]);
	}
}

internal void
R_GraphProcessInvalidateTexture(R_Graph *graph, const GFX_Device *device, R_Pass *pass, const R_PassTextureEdge *edge)
{
	R_GraphTexture *t = R_GraphTextureFromHandle(graph, edge->handle.value);

	if (GFX_TextureKeyIsNull(t->physical_key))
		return;

	const VkImageLayout target_layout = VK_IMAGE_LAYOUT_GENERAL;

	R_ResourceState *st = &t->state;
	
	b32 layout_change = st->layout != target_layout;
	b32 needs_sync = layout_change || (st->to_flush != 0) || R_ResourceNeedsInvalidation(edge->state, st);

	if (needs_sync)
	{
		GFX_AccessSt src_state = {0};
		src_state.stage = st->stage ? st->stage : VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
		src_stage.access = st->to_flush;
		
		const GFX_Texture *physical_texture = GFX_DeviceTextureFromKey(t->physical_key);

		pass->texture_barriers[pass->texture_barrier_count] = GFX_SyncTextureBarrier(physical_texture,
																					 &src_state,
																					 &edge->state,
																					 st->layout,
																					 target_layout,
																					 0, VK_REMAINING_MIP_LEVELS,
																					 0, VK_REMAINING_ARRAY_LAYERS);
		
		pass->texture_barrier_count++;

		AssertTrue(pass->texture_barrier_count < ArraySize(pass->texture_barriers));
		
		st->layout = target_layout;
		st->to_flush = 0;

		if (layout_change || st->to_flush)
			MemZeroArray(st->invalidated_in_stage);

		if (layout_change)
		{
			const u64 dst_stages = edge->state.stage;

			for (u32 i = 0; i < ArraySize(st->invalidated_in_stage); i++)
			{
				if ((dst_stages >> i) & 1)
					st->invalidated_in_stage[i] |= dst_stages;
			}
		}
	}

	st->stage = edge->state.stage;
}

internal void
R_GraphProcessInvalidateBuffer(R_Graph *graph, const GFX_Device *device, R_Pass *pass, const R_PassBufferEdge *edge)
{
	R_GraphBuffer *b = R_GraphBufferFromHandle(graph, edge->handle.value);

	if (GFX_BufferKeyIsNull(t->physical_key))
		return;

	R_ResourceState *st = &t->state;
	
	b32 needs_sync = (st->to_flush != 0) || R_ResourceNeedsInvalidation(edge->state, st);

	if (needs_sync)
	{
		GFX_AccessSt src_state = {0};
		src_state.stage = st->stage ? st->stage : VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
		src_stage.access = st->to_flush;
		
		const GFX_Buffer *physical_buffer = GFX_DeviceBufferFromKey(b->physical_key);

		pass->buffer_barriers[pass->buffer_barrier_count] = GFX_SyncbufferBarrier(physical_buffer,
																				  &src_state,
																				  &edge->state,
																				  st->layout,
																				  buffer_offset,
																				  b->buffer_info.size);
		
		pass->buffer_barrier_count++;

		AssertTrue(pass->buffer_barrier_count < ArraySize(pass->buffer_barriers));
		
		st->to_flush = 0;

		if (layout_change || st->to_flush)
			MemZeroArray(st->invalidated_in_stage);

		if (layout_change)
		{
			const u64 dst_stages = edge->state.stage;

			for (u32 i = 0; i < ArraySize(st->invalidated_in_stage); i++)
			{
				if ((dst_stages >> i) & 1)
					st->invalidated_in_stage[i] |= dst_stages;
			}
		}
	}

	st->stage = edge->state.stage;
}

internal void
R_GraphProcessFlushTexture(R_Graph *graph, const R_PassTextureEdge *edge)
{
	R_GraphTexture *t = R_GraphTextureFromHandle(graph, edge->handle);

	if (GFX_TextureKeyIsNull(t->physical_key))
		return;

	t->state.stage = edge->state.stage;
	t->state.to_flush = edge->state.access;
}

internal void
R_GraphProcessFlushBuffer(R_Graph *graph, const R_PassBufferEdge *edge)
{
	R_GraphBuffer *b = R_GraphBufferFromHandle(graph, edge->handle);

	if (GFX_BufferKeyIsNull(b->physical_key))
		return;

	b->state.stage = edge->state.stage;
	b->state.to_flush = edge->state.access;
}

internal void
R_GraphExecute(R_Graph *graph,
			   GFX_Device *device,
			   const GFX_Swapchain *swapchain,
			   GFX_CmdBuffer *cmd,
			   const R_Scene *scene,
			   const R_Camera *camera,
			   f32 delta_time, f32 elapsed_time)
{
	if (graph->pass_count <= 0)
		return;

	for (u32 pass_index = 0; pass_index < graph->pass_count; pass_index++)
	{
		R_Pass *pass = &graph->passes[pass_index];

		if (pass->is_culled)
			continue;

		DebugLogF("Executing Render Pass: %.*s", pass->name.len, pass->name.str);

		GFX_CmdPipelineBarrier(cmd, 0,
							   pass->memory_barrier_count, pass->memory_barriers,
							   pass->buffer_barrier_count, pass->buffer_barriers,
							   pass->texture_barrier_count, pass->texture_barriers);

		R_PassContext ctx = {0};
		ctx.graph = graph;
		ctx.device = device;
		ctx.cmd = cmd;
		ctx.scene = scene;
		ctx.camera = camera;
		ctx.delta_time = delta_time;
		ctx.elapsed_time = elapsed_time;
		ctx.user_data = pass->user_data;

		if (stage.type == R_PassType_Graphics)
		{
			GFX_RenderInfo render_info = R_GraphBuildRenderingInfo(graph, device, pass);
			
			GFX_CmdBeginRendering(cmd, &render_info);
			pass->record(&ctx);
			GFX_CmdEndRendering(cmd, &render_info);
		}
		else
		{
			pass->record(&ctx);
		}

		// TODO: Here's where we would release resources if doing
		//       a garbage collected pool.
	}

	R_GraphPresentToSwapchain(graph, device, swapchain, cmd);
}

internal void
R_GraphPresentToSwapchain(R_Graph *graph,
						  const GFX_Device *device,
						  const GFX_Swapchain *swapchain,
						  const GFX_CmdBuffer *cmd)
{
	const R_GraphTexture *backbuffer_resource = R_GraphTextureFromHandle(graph, graph->backbuffer_handle);

	const GFX_Texture *swapchain_src_texture = GFX_DeviceTextureFromKey(device, backbuffer_resource->physical_key);
	const GFX_Texture *swapchain_dst_texture = GFX_SwapchainCurrentTexture(swapchain);

	GFX_AccessSt src_src = {
		backbuffer_resource->state.stage,
		backbuffer_resource->state.to_flush
	};

	GFX_AccessSt src_dst = {
		VK_PIPELINE_STAGE_2_BLIT_BIT,
		VK_ACCESS_2_TRANSFER_READ_BIT
	};

	GFX_AccessSt dst_src = {
		VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
		VK_ACCESS_2_NONE
	};

	GFX_AccesSt dst_dst = {
		VK_PIPELINE_STAGE_2_BLIT_BIT,
		VK_ACCESS_2_TRANSFER_WRITE_BIT
	};
	
	VkImageMemoryBarrier2 image_barriers[2] = {0};

	image_barriers[0] = GFX_SyncTextureBarrier(swapchain_src_texture,
											   &src_src,
											   &src_dst,
											   backbuffer_resource->state.layout,
											   VK_IMAGE_LAYOUT_GENERAL,
											   0, VK_REMAINING_MIP_LEVELS,
											   0, VK_REMAINING_ARRAY_LAYERS);
	
	image_barriers[0] = GFX_SyncTextureBarrier(swapchain_dst_texture,
											   &dst_src,
											   &dst_dst,
											   VK_IMAGE_LAYOUT_UNDEFINED,
											   VK_IMAGE_LAYOUT_GENERAL,
											   0, VK_REMAINING_MIP_LEVELS,
											   0, VK_REMAINING_ARRAY_LAYERS);

	GFX_CmdPipelineBarrier(cmd, 0,
						   0, NULL,
						   0, NULL,
						   ArraySize(image_barriers), image_barriers);


	VkImageBlit2 blit = {0};
	blit.sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2;
	
	blit.srcOffsets[0] = (VkOffset3D) { 0, 0, 0 };
	blit.srcOffsets[1] = (VkOffset3D) { (i32)swapchain_src_texture->width, (i32)swapchain_src_texture->height, 1 };
	
	blit.dstOffsets[0] = (VkOffset3D) { 0, 0, 0 };
	blit.dstOffsets[1] = (VkOffset3D) { (i32)swapchain_dst_texture->width, (i32)swapchain_dst_texture->height, 1 };
	
	blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	blit.srcSubresource.mipLevel = 0;
	blit.srcSubresource.baseArrayLayer = 0;
	blit.srcSubresource.layerCount = 1;

	blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	blit.dstSubresource.mipLevel = 0;
	blit.dstSubresource.baseArrayLayer = 0;
	blit.dstSubresource.layerCount = 1;

	GFX_CmdBlit(cmd,
				swapchain_src_texture,
				swapchain_dst_texture,
				1, &blit,
				VK_FILTER_LINEAR);

	GFX_AccessSt present_src = {
		VK_PIPELINE_STAGE_2_TRANSFER_BIT,
		VK_ACCESS_2_TRANSFER_WRITE_BIT
	};

	GFX_AccessSt present_dst = {
		VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
		VK_ACCESS_2_NONE
	};
	
	VkImageMemoryBarrier2 present_barrier = GFX_SyncTextureBarrier(swapchain_dst_texture,
																   &present_src,
																   &present_dst,
																   VK_IMAGE_LAYOUT_GENERAL,
																   VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
																   0, 1,
																   0, 1);

	GFX_CmdPipelineBarrier(cmd, 0,
						   0, NULL,
						   0, NULL,
						   1, &present_barrier);
}

internal const GFX_Texture *
R_GraphResolveTexture(const R_Graph *graph,
					  const GFX_Device *device,
					  R_GraphTexHandle handle)
{
	const R_GraphTexture *graph_texture = R_GraphTextureFromHandle(graph, handle);

	GFX_TextureKey physical_key = graph_texture->physical_key;
	const GFX_Texture *physical_texture = GFX_DeviceTextureFromKey(device, physical_key);
	
	return physical_texture;
}

internal const GFX_TextureView *
R_GraphResolveTextureView(const R_Graph *graph,
						  const GFX_Device *device,
						  R_GraphTexHandle key,
						  GFX_SubresourceRange range)
{
	const R_GraphTexture *graph_texture = R_GraphTextureFromHandle(graph, handle);

	GFX_TextureViewCreateInfo create_info = {0};
	create_info.texture = graph_texture->physical_key;
	create_info.type = GFX_TextureDefaultViewType(graph_texture);
	create_info.range = range;
	
	GFX_TextureViewKey physical_key = GFX_DeviceTextureViewCreate(device, &create_info);
	const GFX_TextureView *physical_view = GFX_DeviceTextureViewFromKey(device, physical_key);
	
	return physical_view;
}

internal const GFX_Buffer *
R_GraphResolveBuffer(const R_Graph *graph,
					 const GFX_Device *device,
					 R_GraphBufHandle key)
{
	const R_GraphBuffer *graph_buffer = R_GraphBufferFromHandle(graph, handle);

	GFX_BufferKey physical_key = graph_buffer->physical_key;
	const GFX_Buffer *physical_buffer = GFX_DeviceBufferFromKey(device, physical_key);
	
	return physical_buffer;
}

internal GFX_BufferRange
R_GraphResolveBufferRange(const R_Graph *graph,
						  const GFX_Device *device,
						  R_GraphBufHandle key)
{
	// TODO

	AssertTrue(false);
	
	return (GFX_BufferRange) {0};
}

internal GFX_RenderInfo
R_GraphBuildRenderingInfo(const R_Graph *graph, const GFX_Device *device, const R_Pass *pass)
{
	GFX_RenderInfo render_info = {0};

	render_info.view_mask = pass->multi_view_mask;
	
	for (u32 i = 0; i < pass->output_count; i++)
	{
		R_PassTextureEdge *out = &pass->outputs[i];
		const R_Clear *clear = &out->clear;
		const R_TextureInfo *attachment_info = &R_GraphTextureFromHandle(graph, out->handle)->texture_info;

		// TODO: Right now it's just based on the last attachments sample count_offset.
		//       Assumption is that all attachments will already have the same sample count_offset.
		//       --> Ideally I should have resolving implemented so they automatically have their resolves.

		render_info.samples = attachment_info->samples;

		render_info.width  = MaxValue(1u, (u32)attachment_info->size_x >> out->range.base_mip);
		render_info.height = MaxValue(1u, (u32)attachment_info->size_y >> out->range.base_mip);

		VkRenderingAttachmentInfo vk_attachment_info = {0};
		vk_attachment_info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		vk_attachment_info.loadOp = output.clear_enabled ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
		vk_attachment_info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		vk_attachment_info.imageView = get_texture_view(output.handle, output.range)->get_handle();
		vk_attachment_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		vk_attachment_info.resolveImageView = VK_NULL_HANDLE; // TODO: MSAA isn't supported yet.
		vk_attachment_info.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		vk_attachment_info.resolveMode = VK_RESOLVE_MODE_NONE;

		if (attachment_info.format == device->context->depth_format)
		{
			vk_attachment_info.clearValue.depthStencil = {
				clear.depth,
				clear.stencil
			};

			render_info.depth_attachment = vk_attachment_info;
		}
		else
		{
			vk_attachment_info.clearValue.color = {
				clear.r,
				clear.g,
				clear.b,
				clear.a
			};

			render_info.colour_attachments[render_info.colour_attachment_count] = vk_attachment_info;
			render_info.colour_attachment_count++;

			AssertTrue(render_info.colour_attachment_count < ArraySize(render_info.colour_attachments));
		}
	}

	return render_info;
}

internal R_GraphTexture *
R_GraphTextureFromHandle(R_Graph *graph, R_GraphTexHandle handle)
{
	AssertTrue(!R_GraphTexHandleIsNull(handle));
	return &graph->texture_res[handle.value];
}

internal R_GraphBuffer *
R_GraphBufferFromHandle(R_Graph *graph, R_GraphBufHandle handle)
{
	AssertTrue(!R_GraphBufHandleIsNull(handle));
	return &graph->buffer_res[handle.value];
}

internal b32
R_GraphTexVersionIsUnwritten(const R_Graph *graph, R_GraphTexHandle handle)
{
	// TODO
}

internal b32
R_GraphBufVersionIsUnwritten(const R_Graph *graph, R_GraphBufHandle handle)
{
	// TODO
}
