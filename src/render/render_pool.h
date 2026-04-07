#ifndef RENDER_POOL_H
#define RENDER_POOL_H

typedef struct R_PooledTexture R_PooledTexture;
struct R_PooledTexture
{
	GFX_TextureKey physical_texture;
	R_TextureInfo info;
	b32 in_use;
	u64 last_time_used;
	R_ResourceState state;
};

typedef struct R_PooledBuffer R_PooledBuffer;
struct R_PooledBuffer
{
	GFX_BufferKey physical_buffer;
	R_BufferInfo info;
	u32 in_use;
	u64 last_time_used;
	R_ResourceState state;
};

typedef struct R_ResourcePool R_ResourcePool;
struct R_ResourcePool
{
	u64 current_time;
	u64 gpu_completed_time;

	u32 texture_capacity;
	u32 texture_count;
	R_PooledTexture *textures;

	u32 buffer_capacity;
	u32 buffer_count;
	R_PooledBuffer *buffers;
};

internal void R_ResourcePoolInit(R_ResourcePool *pool, Arena *arena, u32 max_textures, u32 max_buffers);
internal void R_ResourcePoolDestroy(R_ResourcePool *pool, GFX_Device *device);

// Update timeline values and reset used state for all entries.
// Typically call once per frame.
internal void R_ResourcePoolFlush(R_ResourcePool *pool, const GFX_Device *device);

internal const GFX_Texture *R_ResourcePoolAcquireTexture(R_ResourcePool *pool,
														 GFX_Device *device,
														 const R_TextureInfo *info,
														 R_ResourceState *out_state);

internal const GFX_Buffer *R_ResourcePoolAcquireBuffer(R_ResourcePool *pool,
													   GFX_Device *device,
													   const R_BufferInfo *info,
													   R_ResourceState *out_state);

internal void R_ResourcePoolUpdateTexture(R_ResourcePool *pool,
										  GFX_TextureKey key,
										  const R_ResourceState *state);

internal void R_ResourcePoolUpdateBuffer(R_ResourcePool *pool,
										 GFX_BufferKey key,
										 const R_ResourceState *state);

#endif // RENDER_POOL_H
