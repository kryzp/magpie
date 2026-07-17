#ifndef GRAPHICS_UTIL_H
#define GRAPHICS_UTIL_H

static inline u32 G_ComputeGroupCount(u32 count, u32 tile)
{
	return (count + tile - 1) / tile;
}

static inline u32 G_ClampMipmapCount(u32 mipmaps, u32 w, u32 h, u32 d)
{
	u32 max_size = MaxValue(w, MaxValue(h, d));
	u32 max_mips = 1 + (u32)Log2F((f32)max_size);
	
	return MinValue(mipmaps, max_mips);
}

#endif // GRAPHICS_UTIL_H
