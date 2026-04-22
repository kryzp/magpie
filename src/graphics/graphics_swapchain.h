#ifndef GRAPHICS_SWAPCHAIN_H
#define GRAPHICS_SWAPCHAIN_H

#define GFX_FRAMES_IN_FLIGHT 3

typedef struct GFX_Swapchain GFX_Swapchain;
struct GFX_Swapchain
{
	VkSwapchainKHR handle;

	/*
	 * This is DIFFERENT from the device's current_frame_index.
	 * A swapchain might have, e.g: 3 frames while the graphics
	 * device only has 2 frames in flight. They are *usually* the same
	 * but not always!
	 */
	u32 current_texture_index;

	u32 texture_count;
	GFX_TextureKey *textures;
	GFX_TextureView *views;

	u32 width;
	u32 height;

	VkFormat format;
};

internal inline GFX_TextureKey
GFX_SwapchainCurrentTexture(const GFX_Swapchain *swapchain)
{
	return swapchain->textures[swapchain->current_texture_index];
}

internal inline GFX_TextureView *
GFX_SwapchainCurrentView(const GFX_Swapchain *swapchain)
{
	return &swapchain->views[swapchain->current_texture_index];
}

#endif // GRAPHICS_SWAPCHAIN_H
