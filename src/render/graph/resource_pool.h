#ifndef RENDER_GRAPH_RESOURCE_POOL_H
#define RENDER_GRAPH_RESOURCE_POOL_H

#include "core/types.h"
#include "graphics/resource_key.h"
#include "render/graph/graph_types.h"

#include "container/vector.h"

namespace rnd {

struct PooledTexture {
	gfx::ResourceKey key;
	TextureInfo info;
	bool in_use = false;
	u64 last_frame_used = 0;
	ResourceState state;
};

struct PooledBuffer {
	gfx::ResourceKey key;
	BufferInfo info;
	bool in_use = false;
	u64 last_frame_used = 0;
	ResourceState state;
};

class ResourcePool {
public:
	ResourcePool();
	~ResourcePool();

	ResourcePool(const ResourcePool &) = delete;
	ResourcePool &operator=(const ResourcePool &) = delete;

	void Flush();

	gfx::ResourceKey AcquireTexture(const TextureInfo &info, ResourceState *out_state);
	gfx::ResourceKey AcquireBuffer(const BufferInfo &info, ResourceState *out_state);

	void UpdateTexture(gfx::ResourceKey key, const ResourceState &state);
	void UpdateBuffer(gfx::ResourceKey key, const ResourceState &state);

private:
	u64 current_frame;
	u64 gpu_completed_time;

	Vector<PooledTexture> textures;
	Vector<PooledBuffer> buffers;
};

}

#endif // RENDER_GRAPH_RESOURCE_POOL_H
