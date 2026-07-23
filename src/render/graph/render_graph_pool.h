#ifndef RENDER_GRAPH_POOL_H
#define RENDER_GRAPH_POOL_H

typedef struct R_PooledTexture R_PooledTexture;
struct R_PooledTexture
{
	G_ResourceKey key;
	R_TextureInfo info;
	b32 in_use;
	u64 last_frame_used;
	R_ResourceState state;
};

typedef struct R_PooledBuffer R_PooledBuffer;
struct R_PooledBuffer
{
	G_ResourceKey key;
	R_BufferInfo info;
	u32 in_use;
	u64 last_frame_used;
	R_ResourceState state;
};

typedef struct R_ResourcePool R_ResourcePool;
struct R_ResourcePool
{
	u64 current_frame;
	u64 gpu_completed_time;

	u32 texture_capacity;
	u32 texture_count;
	R_PooledTexture *textures;

	u32 buffer_capacity;
	u32 buffer_count;
	R_PooledBuffer *buffers;
};

internal void R_ResourcePoolInit(R_ResourcePool *pool, Arena *arena, u32 max_textures, u32 max_buffers);
internal void R_ResourcePoolDestroy(R_ResourcePool *pool);

// Update timeline values and reset used state for all entries.
// Typically call once per frame.
internal void R_ResourcePoolFlush(R_ResourcePool *pool);

internal G_ResourceKey R_ResourcePoolAcquireTexture(R_ResourcePool *pool,
												   const R_TextureInfo *info,
												   R_ResourceState *out_state);

internal G_ResourceKey R_ResourcePoolAcquireBuffer(R_ResourcePool *pool,
												 const R_BufferInfo *info,
												 R_ResourceState *out_state);

internal void R_ResourcePoolUpdateTexture(R_ResourcePool *pool,
										  G_ResourceKey key,
										  const R_ResourceState *state);

internal void R_ResourcePoolUpdateBuffer(R_ResourcePool *pool,
										 G_ResourceKey key,
										 const R_ResourceState *state);

#endif // RENDER_GRAPH_POOL_H
