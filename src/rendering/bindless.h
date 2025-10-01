#ifndef GFX_BINDLESS_H
#define GFX_BINDLESS_H

#include <volk/volk.h>

#include "core/core_types.h"

typedef u32 gfx_bindless_handle;

#define GFX_BINDLESS_MAX_RESOURCES 256
#define GFX_BINDLESS_MAX_WRITES_PER_FRAME 64

enum gfx_bindless_set_kind {
	GFX_BINDLESS_SET_sampler,
	GFX_BINDLESS_SET_sampled,
	GFX_BINDLESS_SET_storage,
	GFX_BINDLESS_SET_max_enum
};

struct gfx_bindless_sampler {
	gfx_bindless_handle id;
};

struct gfx_bindless_view {
	gfx_bindless_handle sampled;
	gfx_bindless_handle storage;
};

struct gfx_bindless_update {
	enum gfx_bindless_set_kind kind;
	gfx_bindless_handle slot;
	VkSampler sampler;
	VkImageView view;
};	

struct gfx_bindless {
	VkDescriptorPool pool;
	VkDescriptorSetLayout layouts[GFX_BINDLESS_SET_max_enum];
	VkDescriptorSet sets[GFX_BINDLESS_SET_max_enum];

	u32 resource_counts[GFX_BINDLESS_SET_max_enum];

	u32 update_count;
	struct gfx_bindless_update updates[GFX_BINDLESS_MAX_WRITES_PER_FRAME];
};

gfx_bindless_handle gfx_bindless_push_update(struct gfx_bindless *bindless,
					     enum gfx_bindless_set_kind kind,
					     VkSampler sampler, VkImageView view);

bool gfx_bindless_is_valid(gfx_bindless_handle handle);

struct gfx_bindless_sampler gfx_bindless_register_sampler(struct gfx_bindless *bindless, VkSampler sampler);
struct gfx_bindless_view gfx_bindless_register_view(struct gfx_bindless *bindless, VkImageView view, bool is_sampled, bool is_storage);

#endif // GFX_BINDLESS_H
