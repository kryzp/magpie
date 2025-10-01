#ifndef GFX_GBUFFER_H
#define GFX_GBUFFER_H

#include "device.h"
#include "swapchain.h"

enum gfx_gbuffer_attachment {
	GFX_GBUFFER_ATTACHMENT_position,
	GFX_GBUFFER_ATTACHMENT_albedo,
	GFX_GBUFFER_ATTACHMENT_normal,
	GFX_GBUFFER_ATTACHMENT_metallic_roughness,
	GFX_GBUFFER_ATTACHMENT_emissive,
	GFX_GBUFFER_ATTACHMENT_max_enum
};

struct gfx_gbuffer {
	struct gfx_texture attachments[GFX_GBUFFER_ATTACHMENT_max_enum];
	struct gfx_texture depth;
	struct gfx_texture_view *views[GFX_GBUFFER_ATTACHMENT_max_enum];
	struct gfx_texture_view *depth_view;
};

void gfx_gbuffer_init(struct gfx_gbuffer *buffer, struct gfx_device *device, struct gfx_swapchain *swapchain);
void gfx_gbuffer_destroy(struct gfx_gbuffer *buffer, struct gfx_device *device);

#endif // GFX_GBUFFER_H
