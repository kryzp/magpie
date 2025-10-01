#ifndef GFX_TEXTURE_H
#define GFX_TEXTURE_H

#include <volk/volk.h>
#include <vma/vk_mem_alloc.h>

#include "core/core_types.h"

#include "bindless.h"

enum gfx_texture_access_type {
	GFX_TEXTURE_ACCESS_TYPE_undefined,
	GFX_TEXTURE_ACCESS_TYPE_general,
	GFX_TEXTURE_ACCESS_TYPE_graphics_r,
	GFX_TEXTURE_ACCESS_TYPE_graphics_rw,
	GFX_TEXTURE_ACCESS_TYPE_compute_r,
	GFX_TEXTURE_ACCESS_TYPE_compute_rw,
	GFX_TEXTURE_ACCESS_TYPE_colour,
	GFX_TEXTURE_ACCESS_TYPE_depth,
	GFX_TEXTURE_ACCESS_TYPE_blit_src,
	GFX_TEXTURE_ACCESS_TYPE_blit_dst,
	GFX_TEXTURE_ACCESS_TYPE_copy_src,
	GFX_TEXTURE_ACCESS_TYPE_copy_dst,
	GFX_TEXTURE_ACCESS_TYPE_present,
	GFX_TEXTURE_ACCESS_TYPE_max_enum
};

struct gfx_texture_access {
	VkImageLayout layout;
	VkPipelineStageFlags2 stage;
	VkAccessFlags2 access;
};

struct gfx_texture {
	VkImage handle;
	VkImageUsageFlags usage;

	u32 access_count;

	// Access types are arranged into a 3D matrix: MIPS x LAYERS x ASPECTS.
	// TODO: Use a proper blobbing algorithm for generating pipeline barriers.
	//       Right now I just go through the columns of the matrix.
	//       --> Could be a fun project.
	enum gfx_texture_access_type *access_types;

	u32 width;
	u32 height;
	u32 depth;

	bool is_depth;
	bool is_cubemap;
	bool is_storage;
	bool is_swapchain;

	VkFormat format;
	VkImageViewType type;
	VkImageTiling tiling;

	u32 aspect_count;
	VkImageAspectFlags aspect_flags;
	
	u32 mipmap_count;
	
	VkSampleCountFlagBits samples;
	
	VmaAllocation allocation;
	VmaAllocationInfo allocation_info;
};

u32 gfx_texture_face_count(const struct gfx_texture *texture);
u32 gfx_texture_layer_count(const struct gfx_texture *texture);

enum gfx_texture_access_type gfx_texture_get_access_type(const struct gfx_texture *texture,
							 u32 layer, u32 level, u32 aspect);

void gfx_texture_set_access_type(struct gfx_texture *texture,
				 u32 layer, u32 level, u32 aspect,
				 enum gfx_texture_access_type type);

struct gfx_texture_view {
	VkImageView handle;

	struct gfx_texture *parent;

	struct gfx_bindless_view bindless;
	
	u32 base_layer;
	u32 layer_count;
	
	u32 base_level;
	u32 level_count;

	VkImageAspectFlags aspect;
};

#endif // GFX_TEXTURE_H
