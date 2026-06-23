static void R_ResourcePoolInit(R_ResourcePool *pool, Arena *arena, u32 max_textures, u32 max_buffers)
{
	pool->current_frame = 0;
	pool->gpu_completed_time = 0;
	
	pool->texture_count = 0;
	pool->buffer_count = 0;
	
	pool->texture_capacity = max_textures;
	pool->buffer_capacity = max_buffers;
	
	pool->textures = ArenaPushArray(arena, R_PooledTexture, max_textures);
	pool->buffers = ArenaPushArray(arena, R_PooledBuffer, max_buffers);
}

static void R_ResourcePoolDestroy(R_ResourcePool *pool, G_Device *device)
{
	for (u32 i = 0; i < pool->texture_count; i++)
	{
		R_PooledTexture *t = &pool->textures[i];
		G_DeviceTextureDestroy(device, t->key);
		t->key = G_TextureKeyNull();
	}
	
	for (u32 i = 0; i < pool->buffer_count; i++)
	{
		R_PooledBuffer *b = &pool->buffers[i];
		G_DeviceBufferDestroy(device, b->key);
		b->key = G_BufferKeyNull();
	}

	pool->texture_count = 0;
	pool->buffer_count = 0;
}

// TODO: Implement garbage collection.
// Old incomplete code from C++ era:

/*
	const u64 GARBAGE_COLLECT_THRESHOLD = 120;

	for (auto t = texture_pool.begin(); t != texture_pool.end();) {
		t->in_use = false;

		if (current_frame - t->last_frame_used >= GARBAGE_COLLECT_THRESHOLD) {
			graph.get_device().destroy_texture(t->texture);
			t->texture = nullptr;
			t = texture_pool.erase(t);
		} else {
			t++;
		}
	}

	for (auto b = buffer_pool.begin(); b != buffer_pool.end();) {
		b->in_use = false;

		if (current_frame - b->last_frame_used >= GARBAGE_COLLECT_THRESHOLD) {
			graph.get_device().destroy_buffer(b->buffer);
			b->buffer = nullptr;
			b = buffer_pool.erase(b);
		} else {
			b++;
		}
	}


	
void RenderResourcePool::release_texture(const Texture *texture, const AttachmentInfo &info)
{
	PooledTexture resource = {};
	resource.texture = texture;
	resource.texture_info = info;

	texture_pool.push_back(resource);
}




void RenderResourcePool::release_buffer(const GpuBuffer *buffer, const GpuBufferInfo &info)
{
	PooledBuffer resource = {};
	resource.buffer = buffer;
	resource.buffer_info = info;

	buffer_pool.push_back(resource);
}
*/

static void R_ResourcePoolFlush(R_ResourcePool *pool, const G_Device *device)
{
	pool->current_frame = device->graphics_semaphore.target + 1;
	pool->gpu_completed_time = G_DeviceSemaphoreValue(device, &device->graphics_semaphore);

	for (u32 i = 0; i < pool->texture_count; i++)
	{
		R_PooledTexture *t = &pool->textures[i];
		t->in_use = false;
	}
	
	for (u32 i = 0; i < pool->buffer_count; i++)
	{
		R_PooledBuffer *b = &pool->buffers[i];
		b->in_use = false;
	}
}

static G_TextureKey R_ResourcePoolAcquireTexture(R_ResourcePool *pool,
							 G_Device *device,
							 const R_TextureInfo *info,
							 R_ResourceState *out_state)
{
	for (u32 i = 0; i < pool->texture_count; i++)
	{
		R_PooledTexture *t = &pool->textures[i];
		b32 gpu_done = t->last_frame_used <= pool->gpu_completed_time;

		if (!t->in_use && gpu_done && R_TextureInfoMatch(info, &t->info))
		{
			t->in_use = true;
			t->last_frame_used = pool->current_frame;

			if (out_state)
				*out_state = t->state;

			return t->key;
		}
	}

	G_TextureAllocInfo alloc_info = {0};
	alloc_info.width   = info->size_x;
	alloc_info.height  = info->size_y;
	alloc_info.depth   = info->size_z;
	alloc_info.format  = info->format;
	alloc_info.type    = info->size_z > 1 ? VK_IMAGE_TYPE_3D : VK_IMAGE_TYPE_2D;
	alloc_info.tiling  = VK_IMAGE_TILING_OPTIMAL;
	alloc_info.mipmaps = info->mips;
	alloc_info.layers  = info->layers;
	alloc_info.samples = info->samples;
	alloc_info.flags   = info->flags;
	
	R_PooledTexture texture = {0};
	texture.key = G_DeviceTextureAlloc(device, &alloc_info);
	texture.info = *info;
	texture.in_use = true;
	texture.last_frame_used = pool->current_frame;
	texture.state.write_stage = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
	texture.state.write_access = VK_ACCESS_2_NONE;
	texture.state.read_stages = 0;
	texture.state.layout = VK_IMAGE_LAYOUT_UNDEFINED;

	if (out_state)
		*out_state = texture.state;

	AssertTrue(pool->texture_count < pool->texture_capacity);

	pool->textures[pool->texture_count] = texture;
	pool->texture_count++;

	return texture.key;
}

static G_BufferKey R_ResourcePoolAcquireBuffer(R_ResourcePool *pool,
							G_Device *device,
							const R_BufferInfo *info,
							R_ResourceState *out_state)
{
	for (u32 i = 0; i < pool->buffer_count; i++)
	{
		R_PooledBuffer *b = &pool->buffers[i];
		b32 gpu_done = b->last_frame_used <= pool->gpu_completed_time;

		if (!b->in_use && gpu_done && R_BufferInfoMatch(info, &b->info))
		{
			b->in_use = true;
			b->last_frame_used = pool->current_frame;

			if (out_state)
				*out_state = b->state;

			return b->key;
		}
	}

	G_BufferAllocInfo alloc_info = {0};
	alloc_info.usage = info->usage;
	alloc_info.flags = info->flags;
	alloc_info.size  = info->size;
	
	R_PooledBuffer buffer = {0};
	buffer.key = G_DeviceBufferAlloc(device, &alloc_info);
	buffer.info = *info;
	buffer.in_use = true;
	buffer.last_frame_used = pool->current_frame;
	buffer.state.write_stage = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
	buffer.state.write_access = VK_ACCESS_2_NONE;
	buffer.state.read_stages = 0;

	if (out_state)
		*out_state = buffer.state;

	AssertTrue(pool->buffer_count < pool->buffer_capacity);

	pool->buffers[pool->buffer_count] = buffer;
	pool->buffer_count++;

	return buffer.key;
}

static void R_ResourcePoolUpdateTexture(R_ResourcePool *pool,
							G_TextureKey key,
							const R_ResourceState *state)
{
	for (u32 i = 0; i < pool->texture_count; i++)
	{
		R_PooledTexture *t = &pool->textures[i];

		if (G_TextureKeyMatch(key, t->key))
		{
			t->state = *state;
			return;
		}
	}
}

static void R_ResourcePoolUpdateBuffer(R_ResourcePool *pool,
						   G_BufferKey key,
						   const R_ResourceState *state)
{
	for (u32 i = 0; i < pool->buffer_count; i++)
	{
		R_PooledBuffer *b = &pool->buffers[i];

		if (G_BufferKeyMatch(key, b->key))
		{
			b->state = *state;
			return;
		}
	}
}
