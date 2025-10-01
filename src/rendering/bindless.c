#include "bindless.h"

gfx_bindless_handle gfx_bindless_push_update(struct gfx_bindless *bindless,
					     enum gfx_bindless_set_kind kind,
					     VkSampler sampler, VkImageView view)
{
	assert(bindless->update_count < array_size(bindless->updates));

	struct gfx_bindless_update update = {0};
	update.kind = kind;
	update.sampler = sampler;
	update.view = view;
	update.slot = ++bindless->resource_counts[kind];
	
	bindless->updates[bindless->update_count++] = update;

	return update.slot;
}

bool gfx_bindless_is_valid(gfx_bindless_handle handle)
{
	return handle != 0;
}

struct gfx_bindless_sampler gfx_bindless_register_sampler(struct gfx_bindless *bindless, VkSampler sampler)
{
	struct gfx_bindless_sampler bindless_sampler = {0};

	bindless_sampler.id = gfx_bindless_push_update(bindless, GFX_BINDLESS_SET_sampler, sampler, VK_NULL_HANDLE);

	return bindless_sampler;
}

struct gfx_bindless_view gfx_bindless_register_view(struct gfx_bindless *bindless, VkImageView view, bool is_sampled, bool is_storage)
{
	struct gfx_bindless_view bindless_view = {0};

	if (is_sampled) bindless_view.sampled = gfx_bindless_push_update(bindless, GFX_BINDLESS_SET_sampled, VK_NULL_HANDLE, view);
	if (is_storage) bindless_view.storage = gfx_bindless_push_update(bindless, GFX_BINDLESS_SET_storage, VK_NULL_HANDLE, view);

	return bindless_view;
}
