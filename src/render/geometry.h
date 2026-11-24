#ifndef RENDER_GEOMETRY_H
#define RENDER_GEOMETRY_H

#include "core/types.h"
#include "graphics/mesh.h"
#include "graphics/resource_key.h"

#define RENDER_GEOMETRY_PAGE_VERTEX_BUFFER_SIZE Megabytes(64)
#define RENDER_GEOMETRY_PAGE_INDEX_BUFFER_SIZE  Megabytes(32)

#define RENDER_GEOMETRY_FREE_LIST_MAX_BLOCKS 128

namespace rnd {

struct GeometryFreeBlock {
	u64 offset = 0;
	u64 capacity = 0;
};

class GeometryFreeList {
public:
	GeometryFreeList();
	explicit GeometryFreeList(u64 capacity);

	bool HasAvailable(u64 size) const;
	bool TryAlloc(u64 size, u64 *out_index_offset);
	void Release(u64 offset, u64 size);

	u64 GetTotalFreeBytes() const
	{
		return total_free;
	}

private:
	GeometryFreeBlock blocks[RENDER_GEOMETRY_FREE_LIST_MAX_BLOCKS];
	unsigned count;
	u64 total_free;
};

struct GeometryPage {
	gfx::ResourceKey vertex_buffer;
	gfx::ResourceKey index_buffer;

	unsigned vertex_count = 0;
	unsigned index_count = 0;

	u64 vertex_stride = 0;
	gfx::IndexType index_type = gfx::IndexType_U32;

	unsigned max_vertices = 0;
	unsigned max_indices = 0;

	GeometryFreeList vertex_free;
	GeometryFreeList index_free;
};

}

#endif // RENDER_GEOMETRY_H
