
internal R_GraphTexture *
R_GraphTextureFromHandle(R_Graph *graph, R_GraphTexHandle handle)
{
	AssertTrue(!R_GraphTexHandleIsNull(handle));
	AssertTrue(handle.value <= graph->texture_ver_count);

	u32 ver_index = handle.value - 1;
	u32 res_index = graph->texture_ver[ver_index].resource_index;

	AssertTrue(res_index < graph->texture_res_count);

	return &graph->texture_res[res_index];
}

internal R_GraphBuffer *
R_GraphBufferFromHandle(R_Graph *graph, R_GraphBufHandle handle)
{
	AssertTrue(!R_GraphBufHandleIsNull(handle));
	AssertTrue(handle.value <= graph->buffer_ver_count);

	u32 ver_index = handle.value - 1;
	u32 res_index = graph->buffer_ver[ver_index].resource_index;

	AssertTrue(res_index < graph->buffer_res_count);

	return &graph->buffer_res[res_index];
}

internal b32
R_GraphTexVersionIsUnwritten(const R_Graph *graph, R_GraphTexHandle handle)
{
	AssertTrue(!R_GraphTexHandleIsNull(handle));
	return graph->texture_ver[handle.value - 1].writer_pass == R_GRAPH_INVALID_INDEX;
}

internal b32
R_GraphBufVersionIsUnwritten(const R_Graph *graph, R_GraphBufHandle handle)
{
	AssertTrue(!R_GraphBufHandleIsNull(handle));
	return graph->buffer_ver[handle.value - 1].writer_pass == R_GRAPH_INVALID_INDEX;
}

internal void
R_GraphInit(R_Graph *graph, Arena *arena)
{
	MemZeroStruct(graph);
	
	graph->permanent_arena = arena;

	graph->backbuffer_handle = R_GraphTexHandleNull();

	R_ResourcePoolInit(&graph->pool, graph->permanent_arena, R_GRAPH_MAX_TEX_RESOURCES, R_GRAPH_MAX_BUF_RESOURCES);
}

internal void
R_GraphDestroy(R_Graph *graph, GFX_Device *device)
{
	R_ResourcePoolDestroy(&graph->pool, device);
}

internal void
R_GraphReset(R_Graph *graph, const GFX_Device *device)
{
	for (u32 i = 0; i < graph->texture_res_count; i++)
	{
		R_GraphTexture *t = &graph->texture_res[i];

		if (t->is_imported)
			R_ResourceTrackerSetTexture(&graph->tracker, t->imported_key, t->state);
		else if (!GFX_TextureKeyIsNull(t->physical_key))
			R_ResourcePoolUpdateTexture(&graph->pool, t->physical_key, &t->state);
	}
	
	for (u32 i = 0; i < graph->buffer_res_count; i++)
	{
		R_GraphBuffer *b = &graph->buffer_res[i];

		if (b->is_imported)
			R_ResourceTrackerSetBuffer(&graph->tracker, b->imported_key, b->state);
		else if (!GFX_BufferKeyIsNull(b->physical_key))
			R_ResourcePoolUpdateBuffer(&graph->pool, b->physical_key, &b->state);
	}

	graph->pass_count = 0;

	graph->texture_res_count = 0;
	graph->texture_ver_count = 0;

	graph->buffer_res_count = 0;
	graph->buffer_ver_count = 0;

	graph->imported_texture_count = 0;
	graph->imported_buffer_count = 0;

	graph->backbuffer_handle = R_GraphTexHandleNull();

	R_ResourcePoolFlush(&graph->pool, device);
}

internal R_Pass *
R_GraphAdd(R_Graph *graph, String8 name, R_PassType type)
{
	AssertTrue(graph->pass_count < ArraySize(graph->passes));

	R_Pass *pass = &graph->passes[graph->pass_count];
	
	MemZeroStruct(pass);

	pass->graph = graph;
	
	pass->name = name;
	pass->type = type;
	pass->index = graph->pass_count;
	
	graph->pass_count++;
	
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
	AssertTrue(graph->texture_res_count < ArraySize(graph->texture_res));
	AssertTrue(graph->texture_ver_count < ArraySize(graph->texture_ver));

	u32 res_index = graph->texture_res_count++;
	R_GraphTexture *texture = &graph->texture_res[res_index];
	MemZeroStruct(texture);

	texture->texture_info = *info;
	texture->first_pass_index = R_GRAPH_INVALID_INDEX;
	texture->last_pass_index = R_GRAPH_INVALID_INDEX;
	texture->ref_count = 0;
	texture->is_imported = false;

	u32 ver_index = graph->texture_ver_count++;
	R_GraphTexVersion *version = &graph->texture_ver[ver_index];

	version->resource_index = res_index;
	version->writer_pass = R_GRAPH_INVALID_INDEX;
	version->parent = R_GRAPH_INVALID_INDEX;

	return (R_GraphTexHandle) { ver_index + 1 };
}

internal R_GraphBufHandle
R_GraphCreateBuffer(R_Graph *graph, const R_BufferInfo *info)
{
	AssertTrue(graph->buffer_res_count < ArraySize(graph->buffer_res));
	AssertTrue(graph->buffer_ver_count < ArraySize(graph->buffer_ver));

	u32 res_index = graph->buffer_res_count++;
	R_GraphBuffer *buffer = &graph->buffer_res[res_index];
	MemZeroStruct(buffer);

	buffer->buffer_info = *info;
	buffer->first_pass_index = R_GRAPH_INVALID_INDEX;
	buffer->last_pass_index = R_GRAPH_INVALID_INDEX;
	buffer->ref_count = 0;
	buffer->is_imported = false;

	u32 ver_index = graph->buffer_ver_count++;
	R_GraphBufVersion *version = &graph->buffer_ver[ver_index];

	version->resource_index = res_index;
	version->writer_pass = R_GRAPH_INVALID_INDEX;
	version->parent = R_GRAPH_INVALID_INDEX;

	return (R_GraphBufHandle) { ver_index + 1 };
}

internal R_GraphTexHandle
R_GraphImportTexture(R_Graph *graph, const GFX_Device *device, GFX_TextureKey external_key)
{
	for (u32 i = 0; i < graph->imported_texture_count; i++)
	{
		if (GFX_TextureKeyMatch(graph->imported_textures[i].external_key, external_key))
			return graph->imported_textures[i].handle;
	}

	const GFX_Texture *physical = GFX_DeviceTextureFromKey(device, external_key);
	AssertTrue(physical);

	AssertTrue(graph->texture_res_count < ArraySize(graph->texture_res));
	AssertTrue(graph->texture_ver_count < ArraySize(graph->texture_ver));

	u32 res_index = graph->texture_res_count++;
	R_GraphTexture *texture = &graph->texture_res[res_index];
	MemZeroStruct(texture);

	texture->texture_info.format = physical->format;
	texture->texture_info.size_class = R_SizeClass_Absolute;
	texture->texture_info.size_x = (f32)physical->width;
	texture->texture_info.size_y = (f32)physical->height;
	texture->texture_info.size_z = (f32)physical->depth;
	texture->texture_info.mips = physical->mipmap_count;
	texture->texture_info.layers = physical->layer_count;
	texture->texture_info.samples = physical->sample_count;

	/*
	  texture->texture_info.is_cubemap = (physical->flags & GFX_TextureFlag_Cubemap) != 0;
	  texture->texture_info.is_transient = (physical->flags & GFX_TextureFlag_Transient) != 0;
	  texture->texture_info.is_storage = (physical->flags & GFX_TextureFlag_Storage) != 0;
	*/
	
	texture->physical_key = external_key;
	texture->is_imported = true;
	texture->imported_key = external_key;
	texture->first_pass_index = R_GRAPH_INVALID_INDEX;
	texture->last_pass_index = R_GRAPH_INVALID_INDEX;
	texture->ref_count = 0;

	const R_ResourceState *tracked = R_ResourceTrackerFindTexture(&graph->tracker, external_key);

	if (tracked)
	{
		texture->state = *tracked;
	}
	else
	{
		MemZeroStruct(&texture->state);
		texture->state.write_stage = VK_PIPELINE_STAGE_2_NONE;
		texture->state.layout = VK_IMAGE_LAYOUT_UNDEFINED;
	}

	u32 ver_index = graph->texture_ver_count++;
	R_GraphTexVersion *version = &graph->texture_ver[ver_index];

	version->resource_index = res_index;
	version->writer_pass = R_GRAPH_INVALID_INDEX;
	version->parent = R_GRAPH_INVALID_INDEX;

	R_GraphTexHandle handle = { ver_index + 1 };

	AssertTrue(graph->imported_texture_count < ArraySize(graph->imported_textures));
	R_GraphImportedTexture *entry = &graph->imported_textures[graph->imported_texture_count++];
	entry->external_key = external_key;
	entry->handle = handle;

	return handle;
}

internal R_GraphBufHandle
R_GraphImportBuffer(R_Graph *graph, const GFX_Device *device, GFX_BufferKey external_key)
{
	for (u32 i = 0; i < graph->imported_buffer_count; i++)
	{
		if (GFX_BufferKeyMatch(graph->imported_buffers[i].external_key, external_key))
			return graph->imported_buffers[i].handle;
	}

	const GFX_Buffer *physical = GFX_DeviceBufferFromKey(device, external_key);
	AssertTrue(physical);

	AssertTrue(graph->buffer_res_count < ArraySize(graph->buffer_res));
	AssertTrue(graph->buffer_ver_count < ArraySize(graph->buffer_ver));

	u32 res_index = graph->buffer_res_count++;
	R_GraphBuffer *buffer = &graph->buffer_res[res_index];
	MemZeroStruct(buffer);

	buffer->buffer_info.size = physical->size;
	buffer->buffer_info.flags = physical->allocation_flags;
	buffer->buffer_info.usage = physical->usage;

	buffer->physical_key = external_key;
	buffer->is_imported = true;
	buffer->imported_key = external_key;
	buffer->first_pass_index = R_GRAPH_INVALID_INDEX;
	buffer->last_pass_index = R_GRAPH_INVALID_INDEX;
	buffer->ref_count = 0;

	const R_ResourceState *tracked = R_ResourceTrackerFindBuffer(&graph->tracker, external_key);

	if (tracked)
	{
		buffer->state = *tracked;
	}
	else
	{
		MemZeroStruct(&buffer->state);
		buffer->state.write_stage = VK_PIPELINE_STAGE_2_NONE;
	}

	u32 ver_index = graph->buffer_ver_count++;
	R_GraphBufVersion *version = &graph->buffer_ver[ver_index];

	version->resource_index = res_index;
	version->writer_pass = R_GRAPH_INVALID_INDEX;
	version->parent = R_GRAPH_INVALID_INDEX;

	R_GraphBufHandle handle = { ver_index + 1 };

	AssertTrue(graph->imported_buffer_count < ArraySize(graph->imported_buffers));
	R_GraphImportedBuffer *entry = &graph->imported_buffers[graph->imported_buffer_count++];
	entry->external_key = external_key;
	entry->handle = handle;

	return handle;
}

internal R_GraphTexHandle
R_GraphPushTexVersion(R_Graph *graph, R_GraphTexHandle parent, u32 writer_pass_index)
{
	AssertTrue(!R_GraphTexHandleIsNull(parent));
	AssertTrue(graph->texture_ver_count < ArraySize(graph->texture_ver));

	u32 parent_ver_index = parent.value - 1;
	AssertTrue(parent_ver_index < graph->texture_ver_count);

	R_GraphTexVersion *parent_ver = &graph->texture_ver[parent_ver_index];

	u32 ver_index = graph->texture_ver_count++;
	R_GraphTexVersion *version = &graph->texture_ver[ver_index];

	version->resource_index = parent_ver->resource_index;
	version->writer_pass = writer_pass_index;
	version->parent = parent_ver_index;

	return (R_GraphTexHandle) { ver_index + 1 };
}

internal R_GraphBufHandle
R_GraphPushBufVersion(R_Graph *graph, R_GraphBufHandle parent, u32 writer_pass_index)
{
	AssertTrue(!R_GraphBufHandleIsNull(parent));
	AssertTrue(graph->buffer_ver_count < ArraySize(graph->buffer_ver));

	u32 parent_ver_index = parent.value - 1;
	AssertTrue(parent_ver_index < graph->buffer_ver_count);

	R_GraphBufVersion *parent_ver = &graph->buffer_ver[parent_ver_index];

	u32 ver_index = graph->buffer_ver_count++;
	R_GraphBufVersion *version = &graph->buffer_ver[ver_index];

	version->resource_index = parent_ver->resource_index;
	version->writer_pass = writer_pass_index;
	version->parent = parent_ver_index;

	return (R_GraphBufHandle) { ver_index + 1 };
}

internal void
R_GraphCompile(R_Graph *graph, GFX_Device *device, const GFX_Swapchain *swapchain)
{
	for (u32 i = 0; i < graph->texture_res_count; i++)
	{
		if (graph->texture_res[i].is_imported)
			graph->texture_res[i].ref_count = 1;
	}
	
	for (u32 i = 0; i < graph->buffer_res_count; i++)
	{
		if (graph->buffer_res[i].is_imported)
			graph->buffer_res[i].ref_count = 1;
	}

	if (!R_GraphTexHandleIsNull(graph->backbuffer_handle))
		R_GraphTextureFromHandle(graph, graph->backbuffer_handle)->ref_count++;

	R_GraphPropagateDependencies(graph);
	R_GraphBackpropagateDependencies(graph);
	R_GraphAllocateResources(graph, device, swapchain);
	R_GraphGenerateBarriers(graph, device);
}

internal void
R_GraphPropagateDependencies(R_Graph *graph)
{
	for (u32 i = 0; i < graph->pass_count; i++)
	{
		R_Pass *pass = &graph->passes[i];

		for (u32 j = 0; j < pass->output_texture_count; j++)
		{
			R_GraphTexture *t = R_GraphTextureFromHandle(graph, pass->output_textures[j].handle);
			if (t->first_pass_index == R_GRAPH_INVALID_INDEX)
				t->first_pass_index = i;
		}

		for (u32 j = 0; j < pass->output_buffer_count; j++)
		{
			R_GraphBuffer *b = R_GraphBufferFromHandle(graph, pass->output_buffers[j].handle);
			if (b->first_pass_index == R_GRAPH_INVALID_INDEX)
				b->first_pass_index = i;
		}

		if (pass->is_culled)
			continue;

		for (u32 j = 0; j < pass->input_texture_count; j++)
		{
			R_GraphTexture *t = R_GraphTextureFromHandle(graph, pass->input_textures[j].handle);
			if (t->first_pass_index == R_GRAPH_INVALID_INDEX)
				t->first_pass_index = i;
		}

		for (u32 j = 0; j < pass->input_buffer_count; j++)
		{
			R_GraphBuffer *b = R_GraphBufferFromHandle(graph, pass->input_buffers[j].handle);
			if (b->first_pass_index == R_GRAPH_INVALID_INDEX)
				b->first_pass_index = i;
		}
	}
}

internal void
R_GraphBackpropagateDependencies(R_Graph *graph)
{
	for (i32 i = (i32)graph->pass_count - 1; i >= 0; i--)
	{
		R_Pass *pass = &graph->passes[i];

		pass->is_culled = true;

		for (u32 j = 0; j < pass->output_texture_count; j++)
		{
			R_PassTextureEdge *out = &pass->output_textures[j];
			R_GraphTexture *t = R_GraphTextureFromHandle(graph, out->handle);

			if (out->handle.value == graph->backbuffer_handle.value)
				pass->is_culled = false;

			if (t->ref_count > 0)
				pass->is_culled = false;
			
			if (t->last_pass_index == R_GRAPH_INVALID_INDEX)
				t->last_pass_index = (u32)i;
		}

		for (u32 j = 0; j < pass->output_buffer_count; j++)
		{
			R_PassBufferEdge *out = &pass->output_buffers[j];
			R_GraphBuffer *b = R_GraphBufferFromHandle(graph, out->handle);

			if (b->ref_count > 0)
				pass->is_culled = false;
			
			if (b->last_pass_index == R_GRAPH_INVALID_INDEX)
				b->last_pass_index = (u32)i;
		}

		if (pass->is_culled)
			continue;
		
		for (u32 j = 0; j < pass->input_texture_count; j++)
		{
			R_GraphTexture *t = R_GraphTextureFromHandle(graph, pass->input_textures[j].handle);

			t->ref_count++;

			if (t->last_pass_index == R_GRAPH_INVALID_INDEX)
				t->last_pass_index = (u32)i;
		}

		for (u32 j = 0; j < pass->input_buffer_count; j++)
		{
			R_GraphBuffer *b = R_GraphBufferFromHandle(graph, pass->input_buffers[j].handle);

			b->ref_count++;

			if (b->last_pass_index == R_GRAPH_INVALID_INDEX)
				b->last_pass_index = (u32)i;
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
			R_GraphTexture *t = R_GraphTextureFromHandle(graph, pass->output_textures[j].handle);

			if (t->is_imported)
				continue;

			if (!GFX_TextureKeyIsNull(t->physical_key))
				continue;

			if (t->texture_info.size_class == R_SizeClass_SwapchainRelative)
			{
				t->texture_info.size_x *= (f32)swapchain->width;
				t->texture_info.size_y *= (f32)swapchain->height;
				
				t->texture_info.size_class = R_SizeClass_Absolute;
			}

			t->physical_key = R_ResourcePoolAcquireTexture(&graph->pool, device,
														   &t->texture_info,
														   &t->state);
		}

		for (u32 j = 0; j < pass->output_buffer_count; j++)
		{
			R_GraphBuffer *b = R_GraphBufferFromHandle(graph, pass->output_buffers[j].handle);

			if (b->is_imported)
				continue;

			if (!GFX_BufferKeyIsNull(b->physical_key))
				continue;

			b->physical_key = R_ResourcePoolAcquireBuffer(&graph->pool, device,
														  &b->buffer_info,
														  &b->state);
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

		
		// Reads
		
		for (u32 j = 0; j < pass->input_texture_count; j++)
			R_GraphSyncTextureRead(graph, device, pass, &pass->input_textures[j]);

		for (u32 j = 0; j < pass->input_buffer_count; j++)
			R_GraphSyncBufferRead(graph, device, pass, &pass->input_buffers[j]);

		
		// Writes

		for (u32 j = 0; j < pass->output_texture_count; j++)
			R_GraphSyncTextureWrite(graph, device, pass, &pass->output_textures[j]);

		for (u32 j = 0; j < pass->output_buffer_count; j++)
			R_GraphSyncBufferWrite(graph, device, pass, &pass->output_buffers[j]);
	}
}

internal void
R_GraphSyncTextureRead(R_Graph *graph,
					   const GFX_Device *device,
					   R_Pass *pass,
					   const R_PassTextureEdge *edge)
{
	R_GraphTexture *t = R_GraphTextureFromHandle(graph, edge->handle);

	if (GFX_TextureKeyIsNull(t->physical_key))
		return;

	R_ResourceState *st = &t->state;

	const VkImageLayout target_layout = VK_IMAGE_LAYOUT_GENERAL;
	b32 layout_change = st->layout != target_layout;

	if (st->write_access != 0 || layout_change)
	{
		// RAW HAZARD
		
		AssertTrue(pass->texture_barrier_count < ArraySize(pass->texture_barriers));

		GFX_AccessSt src = {0};
		src.stage  = st->write_stage ? st->write_stage : VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
		src.access = st->write_access;

		const GFX_Texture *physical = GFX_DeviceTextureFromKey(device, t->physical_key);

		pass->texture_barriers[pass->texture_barrier_count++] =
			GFX_SyncTextureBarrier(physical,
								   &src, &edge->state,
								   st->layout, target_layout,
								   0, VK_REMAINING_MIP_LEVELS,
								   0, VK_REMAINING_ARRAY_LAYERS);

		st->layout       = target_layout;
		st->write_access = 0;
		st->read_stages  = edge->state.stage;
	}
	else
	{
		st->read_stages |= edge->state.stage;
	}
}

internal void
R_GraphSyncTextureWrite(R_Graph *graph,
						const GFX_Device *device,
						R_Pass *pass,
						const R_PassTextureEdge *edge)
{
	R_GraphTexture *t = R_GraphTextureFromHandle(graph, edge->handle);

	if (GFX_TextureKeyIsNull(t->physical_key))
		return;

	R_ResourceState *st = &t->state;

	const VkImageLayout target_layout = VK_IMAGE_LAYOUT_GENERAL;
	b32 layout_change = st->layout != target_layout;

	if (st->write_access != 0)
	{
		// WAW HAZARD
		
		AssertTrue(pass->texture_barrier_count < ArraySize(pass->texture_barriers));

		GFX_AccessSt src = {0};
		src.stage  = st->write_stage ? st->write_stage : VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
		src.access = st->write_access;

		const GFX_Texture *physical = GFX_DeviceTextureFromKey(device, t->physical_key);

		pass->texture_barriers[pass->texture_barrier_count++] =
			GFX_SyncTextureBarrier(physical,
								   &src, &edge->state,
								   st->layout, target_layout,
								   0, VK_REMAINING_MIP_LEVELS,
								   0, VK_REMAINING_ARRAY_LAYERS);

		st->layout = target_layout;
	}
	else if (st->read_stages != 0 || layout_change)
	{
		// WAR HAZARD

		AssertTrue(pass->texture_barrier_count < ArraySize(pass->texture_barriers));

		GFX_AccessSt src_access = {0};
		src_access.stage  = st->read_stages ? st->read_stages : VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
		src_access.access = VK_ACCESS_2_NONE;

		GFX_AccessSt dst_access = {0};
		dst_access.stage  = edge->state.stage;
		dst_access.access = layout_change ? edge->state.access : VK_ACCESS_2_NONE;

		const GFX_Texture *physical = GFX_DeviceTextureFromKey(device, t->physical_key);

		pass->texture_barriers[pass->texture_barrier_count++] =
			GFX_SyncTextureBarrier(physical,
								   &src_access,
								   &dst_access,
								   st->layout, target_layout,
								   0, VK_REMAINING_MIP_LEVELS,
								   0, VK_REMAINING_ARRAY_LAYERS);

		st->layout = target_layout;
	}

	st->write_stage  = edge->state.stage;
	st->write_access = edge->state.access & GFX_SYNC_WRITE_ACCESS_MASK;
	st->read_stages  = 0;
}

internal void
R_GraphSyncBufferRead(R_Graph *graph,
					  const GFX_Device *device,
					  R_Pass *pass,
					  const R_PassBufferEdge *edge)
{
	R_GraphBuffer *b = R_GraphBufferFromHandle(graph, edge->handle);

	if (GFX_BufferKeyIsNull(b->physical_key))
		return;

	R_ResourceState *st = &b->state;

	if (st->write_access != 0)
	{
		// RAW HAZARD
		
		AssertTrue(pass->buffer_barrier_count < ArraySize(pass->buffer_barriers));

		GFX_AccessSt src_access = {0};
		src_access.stage  = st->write_stage ? st->write_stage : VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
		src_access.access = st->write_access;

		const GFX_Buffer *physical = GFX_DeviceBufferFromKey(device, b->physical_key);

		pass->buffer_barriers[pass->buffer_barrier_count++] =
			GFX_SyncBufferBarrier(physical,
								  &src_access, &edge->state,
								  edge->offset,
								  edge->size ? edge->size : physical->size);

		st->write_access = 0;
		st->read_stages  = edge->state.stage;
	}
	else
	{
		st->read_stages |= edge->state.stage;
	}
}

internal void
R_GraphSyncBufferWrite(R_Graph *graph,
					   const GFX_Device *device,
					   R_Pass *pass,
					   const R_PassBufferEdge *edge)
{
	R_GraphBuffer *b = R_GraphBufferFromHandle(graph, edge->handle);

	if (GFX_BufferKeyIsNull(b->physical_key))
		return;

	R_ResourceState *st = &b->state;

	if (st->write_access != 0)
	{
		// WAW HAZARD

		AssertTrue(pass->buffer_barrier_count < ArraySize(pass->buffer_barriers));

		GFX_AccessSt src_access = {0};
		src_access.stage  = st->write_stage ? st->write_stage : VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
		src_access.access = st->write_access;

		const GFX_Buffer *physical = GFX_DeviceBufferFromKey(device, b->physical_key);

		pass->buffer_barriers[pass->buffer_barrier_count++] =
			GFX_SyncBufferBarrier(physical,
								  &src_access,
								  &edge->state,
								  edge->offset,
								  edge->size ? edge->size : physical->size);
	}
	else if (st->read_stages != 0)
	{
		// WAR HAZARD

		AssertTrue(pass->buffer_barrier_count < ArraySize(pass->buffer_barriers));

		GFX_AccessSt src_access = {0};
		src_access.stage  = st->read_stages;
		src_access.access = VK_ACCESS_2_NONE;

		GFX_AccessSt dst_access = {0};
		dst_access.stage  = edge->state.stage;
		dst_access.access = VK_ACCESS_2_NONE;

		const GFX_Buffer *physical = GFX_DeviceBufferFromKey(device, b->physical_key);

		pass->buffer_barriers[pass->buffer_barrier_count++] =
			GFX_SyncBufferBarrier(physical,
								  &src_access,
								  &dst_access,
								  edge->offset,
								  edge->size ? edge->size : physical->size);
	}

	st->write_stage  = edge->state.stage;
	st->write_access = edge->state.access & GFX_SYNC_WRITE_ACCESS_MASK;
	st->read_stages  = 0;
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
	for (u32 i = 0; i < graph->pass_count; i++)
	{
		R_Pass *pass = &graph->passes[i];

		if (pass->is_culled)
			continue;

		GFX_CmdPipelineBarrier(cmd, 0,
							   pass->memory_barrier_count,  pass->memory_barriers,
							   pass->buffer_barrier_count,  pass->buffer_barriers,
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

		if (pass->type == R_PassType_Graphics)
		{
			GFX_RenderInfo render_info = R_GraphBuildRenderingInfo(graph, device, pass);
			
			GFX_CmdBeginRendering(cmd, &render_info);

			if (pass->record)
				pass->record(&ctx);

			GFX_CmdEndRendering(cmd);
		}
		else
		{
			if (pass->record)
				pass->record(&ctx);
		}
	}
}

internal void
R_GraphPresentToSwapchain(R_Graph *graph,
						  const GFX_Device *device,
						  const GFX_Swapchain *swapchain,
						  GFX_CmdBuffer *cmd)
{
	if (R_GraphTexHandleIsNull(graph->backbuffer_handle))
		return;

	R_GraphTexture *backbuffer = R_GraphTextureFromHandle(graph, graph->backbuffer_handle);

	const GFX_Texture *src_texture = GFX_DeviceTextureFromKey(device, backbuffer->physical_key);
	const GFX_Texture *dst_texture = GFX_SwapchainCurrentTexture(swapchain);

	// Backbuffer = Transfer Source
	// Swapchain  = Trandfer Destination
	
	GFX_AccessSt src_src = { backbuffer->state.write_stage, backbuffer->state.write_access };
	GFX_AccessSt src_dst = { VK_PIPELINE_STAGE_2_BLIT_BIT, VK_ACCESS_2_TRANSFER_READ_BIT };

	GFX_AccessSt dst_src = { VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, VK_ACCESS_2_NONE };
	GFX_AccessSt dst_dst = { VK_PIPELINE_STAGE_2_BLIT_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT };
	
	VkImageMemoryBarrier2 pre_blit_barriers[2] = {0};

	pre_blit_barriers[0] = GFX_SyncTextureBarrier(src_texture,
												  &src_src,
												  &src_dst,
												  backbuffer->state.layout,
												  VK_IMAGE_LAYOUT_GENERAL,
												  0, VK_REMAINING_MIP_LEVELS,
												  0, VK_REMAINING_ARRAY_LAYERS);
	
	pre_blit_barriers[1] = GFX_SyncTextureBarrier(dst_texture,
												  &dst_src,
												  &dst_dst,
												  VK_IMAGE_LAYOUT_UNDEFINED,
												  VK_IMAGE_LAYOUT_GENERAL,
												  0, VK_REMAINING_MIP_LEVELS,
												  0, VK_REMAINING_ARRAY_LAYERS);

	GFX_CmdPipelineBarrier(cmd, 0,
						   0, NULL,
						   0, NULL,
						   ArraySize(pre_blit_barriers), pre_blit_barriers);

	
	// Blit the backbuffer to the swapchain.

	VkImageBlit2 blit = {0};
	blit.sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2;
	
	blit.srcOffsets[0] = (VkOffset3D) { 0, 0, 0 };
	blit.srcOffsets[1] = (VkOffset3D) { (i32)src_texture->width, (i32)src_texture->height, 1 };
	
	blit.dstOffsets[0] = (VkOffset3D) { 0, 0, 0 };
	blit.dstOffsets[1] = (VkOffset3D) { (i32)dst_texture->width, (i32)dst_texture->height, 1 };
	
	blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	blit.srcSubresource.mipLevel = 0;
	blit.srcSubresource.baseArrayLayer = 0;
	blit.srcSubresource.layerCount = 1;

	blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	blit.dstSubresource.mipLevel = 0;
	blit.dstSubresource.baseArrayLayer = 0;
	blit.dstSubresource.layerCount = 1;

	GFX_CmdBlit(cmd, src_texture, dst_texture, 1, &blit, VK_FILTER_LINEAR);

	
	// Transition swpachain to present.
	
	GFX_AccessSt present_src = { VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT };
	GFX_AccessSt present_dst = { VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT, VK_ACCESS_2_NONE };
	
	VkImageMemoryBarrier2 present_barrier = GFX_SyncTextureBarrier(dst_texture,
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

/*
 * YES I CAST THE (const R_Graph *) TO A (R_Graph *) GET OVER IT
 * ICBA TO WRITE A WHOLE ASS OTHER FUNCTION FOR THAT
 * THE RESOURCE NEVER GETS MODIFIED AS FAR AS I CARE
 */

internal const GFX_Texture *
R_GraphResolveTexture(const R_Graph *graph,
					  const GFX_Device *device,
					  R_GraphTexHandle handle)
{
	const R_GraphTexture *t = R_GraphTextureFromHandle((R_Graph *)graph, handle);
	return GFX_DeviceTextureFromKey(device, t->physical_key);
}

internal const GFX_TextureView *
R_GraphResolveTextureView(const R_Graph *graph,
						  GFX_Device *device,
						  R_GraphTexHandle handle,
						  GFX_SubresourceRange range)
{
	const R_GraphTexture *t = R_GraphTextureFromHandle((R_Graph *)graph, handle);

	const GFX_Texture *physical = GFX_DeviceTextureFromKey(device, t->physical_key);

	GFX_TextureViewCreateInfo create_info = {0};
	create_info.texture = t->physical_key;
	create_info.type = GFX_TextureDefaultViewType(physical);
	create_info.range = range;
	
	GFX_TextureViewKey view_key = GFX_DeviceTextureViewFetch(device, &create_info);

	return GFX_DeviceTextureViewFromKey(device, view_key);
}

internal const GFX_Buffer *
R_GraphResolveBuffer(const R_Graph *graph,
					 const GFX_Device *device,
					 R_GraphBufHandle handle)
{
	const R_GraphBuffer *b = R_GraphBufferFromHandle((R_Graph *)graph, handle);
	return GFX_DeviceBufferFromKey(device, b->physical_key);
}

internal GFX_BufferRange
R_GraphResolveBufferRange(const R_Graph *graph,
						  const GFX_Device *device,
						  R_GraphBufHandle handle)
{
	const R_GraphBuffer *b = R_GraphBufferFromHandle((R_Graph *)graph, handle);
	const GFX_Buffer *physical = GFX_DeviceBufferFromKey(device, b->physical_key);

	GFX_BufferRange range = {0};
	range.buffer = physical;
	range.size = b->buffer_info.size;
	range.offset = 0;

	return range;
}

internal GFX_RenderInfo
R_GraphBuildRenderingInfo(const R_Graph *graph, GFX_Device *device, const R_Pass *pass)
{
	GFX_RenderInfo render_info = {0};
	render_info.view_mask = pass->multi_view_mask;
	
	for (u32 i = 0; i < pass->output_texture_count; i++)
	{
		const R_PassTextureEdge *out = &pass->output_textures[i];
		const R_GraphTexture *t = R_GraphTextureFromHandle((R_Graph *)graph, out->handle);
		const R_TextureInfo *info = &t->texture_info;

		// TODO: right now its just based on the last attachments sample count.
		//       Assumption is that all attachments will already have the same sample count.
		//       --> ideally there should be implicit resolving per attachment when
		//           get around to finally implementing msaa n stuff.
		render_info.samples = info->samples;

		render_info.width  = MaxValue(1u, (u32)info->size_x >> out->attachment_range.base_mip);
		render_info.height = MaxValue(1u, (u32)info->size_y >> out->attachment_range.base_mip);

		const GFX_TextureView *view = R_GraphResolveTextureView(graph, device, out->handle, out->attachment_range);

		VkRenderingAttachmentInfo vk_info = {0};
		vk_info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;

		vk_info.loadOp = out->should_clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
		vk_info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

		vk_info.imageView = view->handle;
		vk_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		// TODO: support MSAA!!!
		vk_info.resolveImageView = VK_NULL_HANDLE;
		vk_info.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		vk_info.resolveMode = VK_RESOLVE_MODE_NONE;

		if (info->format == device->context.depth_format)
		{
			vk_info.clearValue.depthStencil.depth = out->clear.depth;
			vk_info.clearValue.depthStencil.stencil = out->clear.stencil;

			render_info.depth_attachment = vk_info;
		}
		else
		{
			vk_info.clearValue.color.float32[0] = out->clear.r;
			vk_info.clearValue.color.float32[1] = out->clear.g;
			vk_info.clearValue.color.float32[2] = out->clear.b;
			vk_info.clearValue.color.float32[3] = out->clear.a;

			AssertTrue(render_info.colour_attachment_count < ArraySize(render_info.colour_attachments));
			render_info.colour_attachments[render_info.colour_attachment_count++] = vk_info;
		}
	}

	return render_info;
}
