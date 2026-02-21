#pragma once

#include <volk/volk.h>

#include "core/types.h"

namespace gfx
{
	class Texture;
	class GpuBuffer;
	
	struct AccessState {
		VkPipelineStageFlags2 stage;
		VkAccessFlags2 access;
	};

	namespace sync
	{
		inline bool is_write_access(VkAccessFlags2 access_flags)
		{
			constexpr static VkAccessFlags2 WRITE_BITS = 
				VK_ACCESS_2_SHADER_WRITE_BIT |
				VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT |
				VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
				VK_ACCESS_2_TRANSFER_WRITE_BIT |
				VK_ACCESS_2_HOST_WRITE_BIT |
				VK_ACCESS_2_MEMORY_WRITE_BIT |
				VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;

			return (access_flags & WRITE_BITS) != 0;
		}

		VkImageMemoryBarrier2 texture_memory_barrier(
			const Texture *texture,
			const AccessState &src,
			const AccessState &dst,
			VkImageLayout src_layout,
			VkImageLayout dst_layout,
			u32 base_mip, u32 mip_count,     // VK_REMAINING_MIP_LEVELS for all mip levels
			u32 base_layer, u32 layer_count  // VK_REMAINING_ARRAY_LAYERS for all array layers
		);

		VkBufferMemoryBarrier2 buffer_memory_barrier(
			const GpuBuffer *buffer,
			const AccessState &src,
			const AccessState &dst,
			u64 offset, u64 size // VK_WHOLE_SIZE for entire buffer
		);
	}

#if 0
	class Texture;
	class GpuBuffer;
	
	enum TextureAccessType {
		TEXTURE_ACCESS_UNDEFINED,
		
		TEXTURE_ACCESS_GENERAL,
		TEXTURE_ACCESS_COLOUR_ATTACHMENT,
		TEXTURE_ACCESS_DEPTH_ATTACHMENT,
		TEXTURE_ACCESS_DEPTH_ATTACHMENT_READ,
		TEXTURE_ACCESS_SAMPLED,
		TEXTURE_ACCESS_graphics_rw,
		TEXTURE_ACCESS_STORAGE_READ,
		TEXTURE_ACCESS_STORAGE_WRITE,
		TEXTURE_ACCESS_BLIT_SRC,
		TEXTURE_ACCESS_BLIT_DST,
		TEXTURE_ACCESS_COPY_SRC,
		TEXTURE_ACCESS_COPY_DST,
		TEXTURE_ACCESS_PRESENT,

		TEXTURE_ACCESS_MAX_ENUM
	};

	struct TextureAccess {
		VkImageLayout layout;
		VkPipelineStageFlags2 stage;
		VkAccessFlags2 access;
	};

	enum GpuBufferAccessType {
		GPU_BUFFER_ACCESS_UNDEFINED,

		GPU_BUFFER_ACCESS_GRAPHICS_READ_WRITE,
		GPU_BUFFER_ACCESS_COMPUTE_READ_WRITE,
		GPU_BUFFER_ACCESS_INDEX,
		GPU_BUFFER_ACCESS_VERTEX,
		GPU_BUFFER_ACCESS_INDIRECT,
		GPU_BUFFER_ACCESS_TRANSFER_SRC,
		GPU_BUFFER_ACCESS_TRANSFER_DST,
		GPU_BUFFER_ACCESS_COPY_SRC,
		GPU_BUFFER_ACCESS_COPY_DST,

		GPU_BUFFER_ACCESS_MAX_ENUM
	};

	struct GpuBufferAccess {
		VkPipelineStageFlags2 stage;
		VkAccessFlags2 access;
	};

	namespace sync
	{
		bool texture_access_is_write(TextureAccessType type);

		TextureAccess get_src_texture_access(TextureAccessType type);
		TextureAccess get_dst_texture_access(TextureAccessType type);

		GpuBufferAccess get_src_buffer_access(GpuBufferAccessType type);
		GpuBufferAccess get_dst_buffer_access(GpuBufferAccessType type);

		VkImageMemoryBarrier2 texture_memory_barrier(
			const Texture *texture,
			const TextureAccess &src_access_info,
			const TextureAccess &dst_access_info,
			u32 base_mip, u32 mip_count,
			u32 base_layer, u32 layer_count
		);

		VkBufferMemoryBarrier2 buffer_memory_barrier(
			const GpuBuffer *buffer,
			const GpuBufferAccess &src_access_info,
			const GpuBufferAccess &dst_access_info,
			u64 offset, u64 size // VK_WHOLE_SIZE for entire buffer
		);
	}
#endif
}
