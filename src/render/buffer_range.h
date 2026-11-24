#ifndef RENDER_BUFFER_RANGE_H
#define RENDER_BUFFER_RANGE_H

#include "core/types.h"
#include "graphics/resource_key.h"

namespace rnd {

struct BufferRange {
	gfx::ResourceKey buffer;
	u64 size = 0;
	u64 offset = 0;

	void *Map() const;
	u64 Address() const;
};

}

#endif // RENDER_BUFFER_RANGE_H
