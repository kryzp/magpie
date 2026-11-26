#ifndef GRAPHICS_UTIL_H
#define GRAPHICS_UTIL_H

#include "core/types.h"
#include "core/maths.h"

namespace gfx {

inline unsigned ComputeGroupCount(unsigned count, unsigned tile)
{
	return (count + tile - 1) / tile;
}

inline unsigned ClampMipmapCount(unsigned mipmaps, unsigned width, unsigned height, unsigned depth)
{
	unsigned max_size = maths::Max(width, maths::Max(height, depth));
	unsigned max_mips = 1 + static_cast<unsigned>(std::log2(static_cast<float>(max_size)));

	return maths::Min(mipmaps, max_mips);
}

}

#endif // GRAPHICS_UTIL_H
