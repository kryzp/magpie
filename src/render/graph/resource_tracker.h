#ifndef RENDER_GRAPH_RESOURCE_TRACKER_H
#define RENDER_GRAPH_RESOURCE_TRACKER_H

#include "core/types.h"
#include "graphics/resource_key.h"
#include "render/graph/graph_types.h"
#include "container/vector.h"

#define RENDER_TRACKER_MAX_TRACKED 128

namespace rnd {

struct TrackedTexture {
	gfx::ResourceKey key;
	ResourceState state;
};

struct TrackedBuffer {
	gfx::ResourceKey key;
	ResourceState state;
};

class ResourceTracker {
public:
	ResourceTracker();

	const ResourceState *FindTexture(gfx::ResourceKey key) const;
	const ResourceState *FindBuffer(gfx::ResourceKey key) const;

	void SetTexture(gfx::ResourceKey key, const ResourceState &state);
	void SetBuffer(gfx::ResourceKey key, const ResourceState &state);

private:
	Vector<TrackedTexture> textures;
	Vector<TrackedBuffer> buffers;
};

}

#endif // RENDER_GRAPH_RESOURCE_TRACKER_H
