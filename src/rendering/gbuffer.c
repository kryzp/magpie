#include "gbuffer.h"

void gfx_gbuffer_init(struct gfx_gbuffer *gbuffer, struct gfx_device *device, struct gfx_swapchain *swapchain)
{
	for (int i = 0; i < GFX_GBUFFER_ATTACHMENT_max_enum; i++) {
		gbuffer->attachments[i] = gfx_device_texture_alloc_2d(device,
								      swapchain->width,
								      swapchain->height,
								      VK_FORMAT_R32G32B32A32_SFLOAT, 1);
		
		gbuffer->views[i] = gfx_device_texture_view_fetch_std(device, &gbuffer->attachments[i]);
	}
	
	gbuffer->depth = gfx_device_texture_alloc_depth_2d(device,
							   swapchain->width,
							   swapchain->height, 1);
	
	gbuffer->depth_view = gfx_device_texture_view_fetch_std(device, &gbuffer->depth);
}

void gfx_gbuffer_destroy(struct gfx_gbuffer *gbuffer, struct gfx_device *device)
{
	for (int i = 0; i < GFX_GBUFFER_ATTACHMENT_max_enum; i++)
		gfx_device_texture_destroy(device, gbuffer->attachments + i);

	gfx_device_texture_destroy(device, &gbuffer->depth);
}
