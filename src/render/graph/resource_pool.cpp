#include "render/graph/resource_pool.h"
#include "graphics/device.h"

rnd::ResourcePool::ResourcePool()
	: current_frame(0)
	, gpu_completed_time(0)
{
}

rnd::ResourcePool::~ResourcePool()
{
	gfx::Device *device = gfx::Device::Get();

	for (PooledTexture &t : textures) {
		device->TextureDestroy(t.key);
		t.key = gfx::ResourceKey::Null();
	}

	for (PooledBuffer &b : buffers) {
		device->BufferDestroy(b.key);
		b.key = gfx::ResourceKey::Null();
	}

	textures.clear();
	buffers.clear();
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
*/

void rnd::ResourcePool::Flush()
{
	gfx::Device *device = gfx::Device::Get();

	current_frame = device->LastSubmittedFrame() + 1;
	gpu_completed_time = device->GPUCompletedFrame();

	for (PooledTexture &t : textures)
		t.in_use = false;

	for (PooledBuffer &b : buffers)
		b.in_use = false;
}

gfx::ResourceKey rnd::ResourcePool::AcquireTexture(const TextureInfo &info, ResourceState *out_state)
{
	for (PooledTexture &t : textures) {
		bool gpu_done = t.last_frame_used <= gpu_completed_time;

		if (!t.in_use && gpu_done && info == t.info) {
			t.in_use = true;
			t.last_frame_used = current_frame;

			if (out_state)
				*out_state = t.state;

			return t.key;
		}
	}

	gfx::TextureAllocInfo alloc_info;
	alloc_info.width = info.size_x;
	alloc_info.height = info.size_y;
	alloc_info.depth = info.size_z;
	alloc_info.format = info.format;
	alloc_info.type = info.size_z > 1 ? VK_IMAGE_TYPE_3D : VK_IMAGE_TYPE_2D;
	alloc_info.tiling = VK_IMAGE_TILING_OPTIMAL;
	alloc_info.mipmaps = info.mips;
	alloc_info.layers = info.layers;
	alloc_info.samples = info.samples;
	alloc_info.flags = info.flags;

	PooledTexture texture;
	texture.key = gfx::Device::Get()->TextureAlloc(alloc_info);
	texture.info = info;
	texture.in_use = true;
	texture.last_frame_used = current_frame;
	texture.state.write_stage = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
	texture.state.write_access = VK_ACCESS_2_NONE;
	texture.state.read_stages = 0;
	texture.state.layout = VK_IMAGE_LAYOUT_UNDEFINED;

	if (out_state)
		*out_state = texture.state;

	textures.push_back(texture);

	return texture.key;
}

gfx::ResourceKey rnd::ResourcePool::AcquireBuffer(const BufferInfo &info, ResourceState *out_state)
{
	for (PooledBuffer &b : buffers) {
		bool gpu_done = b.last_frame_used <= gpu_completed_time;

		if (!b.in_use && gpu_done && info == b.info) {
			b.in_use = true;
			b.last_frame_used = current_frame;

			if (out_state)
				*out_state = b.state;

			return b.key;
		}
	}

	gfx::BufferAllocInfo alloc_info;
	alloc_info.usage = info.usage;
	alloc_info.flags = info.flags;
	alloc_info.size = info.size;

	PooledBuffer buffer;
	buffer.key = gfx::Device::Get()->BufferAlloc(alloc_info);
	buffer.info = info;
	buffer.in_use = true;
	buffer.last_frame_used = current_frame;
	buffer.state.write_stage = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
	buffer.state.write_access = VK_ACCESS_2_NONE;
	buffer.state.read_stages = 0;

	if (out_state)
		*out_state = buffer.state;

	buffers.push_back(buffer);

	return buffer.key;
}

void rnd::ResourcePool::UpdateTexture(gfx::ResourceKey key, const ResourceState &state)
{
	for (PooledTexture &t : textures) {
		if (key == t.key) {
			t.state = state;
			return;
		}
	}
}

void rnd::ResourcePool::UpdateBuffer(gfx::ResourceKey key, const ResourceState &state)
{
	for (PooledBuffer &b : buffers) {
		if (key == b.key) {
			b.state = state;
			return;
		}
	}
}
