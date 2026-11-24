#ifndef GRAPHICS_SYNC_H
#define GRAPHICS_SYNC_H

#include "core/types.h"
#include "graphics/texture.h"
#include "graphics/buffer.h"

#include <volk/volk.h>

#define GRAPHICS_SYNC_WRITE_ACCESS_MASK               \
	(VK_ACCESS_2_SHADER_WRITE_BIT |                   \
	 VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT |         \
	 VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | \
	 VK_ACCESS_2_TRANSFER_WRITE_BIT |                 \
	 VK_ACCESS_2_HOST_WRITE_BIT |                     \
	 VK_ACCESS_2_MEMORY_WRITE_BIT |                   \
	 VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT)

namespace gfx {

struct AccessState {
	VkPipelineStageFlags2 stage = 0;
	VkAccessFlags2 access = 0;
};

namespace sync {

VkMemoryBarrier2 MemoryBarrier(
	const AccessState &src,
	const AccessState &dst
);

VkImageMemoryBarrier2 TextureBarrier(
	const Texture *texture,
	const AccessState &src,
	const AccessState &dst,
	VkImageLayout src_layout,
	VkImageLayout dst_layout,
	unsigned base_mip, unsigned mip_count,      // VK_REMAINING_MIP_LEVELS
	unsigned base_layer, unsigned layer_count   // VK_REMAINING_ARRAY_LAYERS
);

VkBufferMemoryBarrier2 BufferBarrier(
	const Buffer *buffer,
	const AccessState &src,
	const AccessState &dst,
	u64 offset, u64 size // VK_WHOLE_SIZE
);

}

}

#endif // GRAPHICS_SYNC_H
