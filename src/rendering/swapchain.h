#ifndef GFX_SWAPCHAIN_H
#define GFX_SWAPCHAIN_H

#include <volk/volk.h>

#include "core/core_types.h"

#include "texture.h"

struct gfx_swapchain_support_details {
	VkSurfaceCapabilitiesKHR capabilities;
	u32 surface_format_count;
	VkSurfaceFormatKHR *surface_formats;
	u32 present_mode_count;
	VkPresentModeKHR *present_modes;
};

struct gfx_swapchain {
	VkSwapchainKHR handle;

	// This is *DIFFERENT* from gfx_device::current_frame_index.
	// A swapchain might have, e.g: 3 frames while the graphics
	// device only has 2 frames in flight. They are *usually* the same
	// but not always!
	u32 current_texture_index;

	u32 width;
	u32 height;

	VkFormat format;

	u32 swapchain_texture_count;
	struct gfx_texture *swapchain_textures;
	struct gfx_texture_view *swapchain_views;
};

struct gfx_texture *gfx_swapchain_current_texture(struct gfx_swapchain *swapchain);
struct gfx_texture_view *gfx_swapchain_current_view(struct gfx_swapchain *swapchain);

#endif // GFX_SWAPCHAIN_H
