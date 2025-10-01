#ifndef GFX_SYNC_H
#define GFX_SYNC_H

#include "core/core_types.h"

#include "texture.h"
#include "buffer.h"

struct gfx_texture_access gfx_sync_get_src_texture_access(enum gfx_texture_access_type type);
struct gfx_texture_access gfx_sync_get_dst_texture_access(enum gfx_texture_access_type type);

struct gfx_buffer_access gfx_sync_get_src_buffer_access(enum gfx_buffer_access_type type);
struct gfx_buffer_access gfx_sync_get_dst_buffer_access(enum gfx_buffer_access_type type);

VkImageMemoryBarrier2 gfx_sync_texture_memory_barrier(struct gfx_texture *texture,
						      struct gfx_texture_access src_access_info,
						      struct gfx_texture_access dst_access_info,
						      u32 base_level, u32 level_count,
						      u32 base_layer, u32 layer_count);

VkBufferMemoryBarrier2 gfx_sync_buffer_memory_barrier(struct gfx_buffer *buffer,
						      struct gfx_buffer_access src_access_info,
						      struct gfx_buffer_access dst_access_info);

#endif // GFX_SYNC_H
