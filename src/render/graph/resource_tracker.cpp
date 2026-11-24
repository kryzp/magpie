#include "render/graph/resource_tracker.h"
#include "core/log.h"

rnd::ResourceTracker::ResourceTracker()
	: textures()
	, buffers()
{
}

const rnd::ResourceState *rnd::ResourceTracker::FindTexture(gfx::ResourceKey key) const
{
	for (auto &t : textures) {
		if (key == t.key)
			return &t.state;
	}

	return nullptr;
}

const rnd::ResourceState *rnd::ResourceTracker::FindBuffer(gfx::ResourceKey key) const
{
	for (auto &b : buffers) {
		if (key == b.key)
			return &b.state;
	}

	return nullptr;
}

void rnd::ResourceTracker::SetTexture(gfx::ResourceKey key, const ResourceState &state)
{
	for (auto &t : textures) {
		if (key == t.key) {
			t.state = state;
			return;
		}
	}

	TrackedTexture t;
	t.key = key;
	t.state = state;

	textures.push_back(t);
}

void rnd::ResourceTracker::SetBuffer(gfx::ResourceKey key, const ResourceState &state)
{
	for (auto &b : buffers) {
		if (key == b.key) {
			b.state = state;
			return;
		}
	}

	TrackedBuffer b;
	b.key = key;
	b.state = state;

	buffers.push_back(b);
}
