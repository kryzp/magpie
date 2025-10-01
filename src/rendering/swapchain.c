#include "swapchain.h"

struct gfx_texture *gfx_swapchain_current_texture(struct gfx_swapchain *swapchain)
{
	return swapchain->swapchain_textures + swapchain->current_texture_index;
}

struct gfx_texture_view *gfx_swapchain_current_view(struct gfx_swapchain *swapchain)
{
	return swapchain->swapchain_views + swapchain->current_texture_index;
}
