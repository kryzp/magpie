#pragma once

#include <volk/volk.h>

#include "core/types.h"

namespace gfx
{

class Texture;
class GpuBuffer;

namespace sync
{

// TODO: ts code pmo, genuinely slime it.

enum TextureAccessType {
	TEXTURE_ACCESS_undefined,
	TEXTURE_ACCESS_general,
	TEXTURE_ACCESS_graphics_r,
	TEXTURE_ACCESS_graphics_rw,
	TEXTURE_ACCESS_compute_r,
	TEXTURE_ACCESS_compute_rw,
	TEXTURE_ACCESS_colour,
	TEXTURE_ACCESS_depth,
	TEXTURE_ACCESS_blit_src,
	TEXTURE_ACCESS_blit_dst,
	TEXTURE_ACCESS_copy_src,
	TEXTURE_ACCESS_copy_dst,
	TEXTURE_ACCESS_present,
	TEXTURE_ACCESS_max_enum
};

struct TextureAccess {
	VkImageLayout layout;
	VkPipelineStageFlags2 stage;
	VkAccessFlags2 access;
};

enum GpuBufferAccessType {
	GPU_BUFFER_ACCESS_undefined,
	GPU_BUFFER_ACCESS_graphics_rw,
	GPU_BUFFER_ACCESS_compute_rw,
	GPU_BUFFER_ACCESS_copy_src,
	GPU_BUFFER_ACCESS_copy_dst,
	GPU_BUFFER_ACCESS_indirect,
	GPU_BUFFER_ACCESS_max_enum
};

struct GpuBufferAccess {
	VkPipelineStageFlags2 stage;
	VkAccessFlags2 access;
};

TextureAccess get_src_texture_access(TextureAccessType type);
TextureAccess get_dst_texture_access(TextureAccessType type);

GpuBufferAccess get_src_buffer_access(GpuBufferAccessType type);
GpuBufferAccess get_dst_buffer_access(GpuBufferAccessType type);

VkImageMemoryBarrier2 texture_memory_barrier(
	const Texture &texture,
	const TextureAccess &src_access_info,
	const TextureAccess &dst_access_info,
	u32 base_mip, u32 mip_count,
	u32 base_layer, u32 layer_count
);

VkBufferMemoryBarrier2 buffer_memory_barrier(
	const GpuBuffer &buffer,
	const GpuBufferAccess &src_access_info,
	const GpuBufferAccess &dst_access_info
);

}

}
