#ifndef GRAPHICS_SWAPCHAIN_H
#define GRAPHICS_SWAPCHAIN_H

#define G_FRAMES_IN_FLIGHT 3

typedef struct G_Swapchain G_Swapchain;
struct G_Swapchain
{
	VkSwapchainKHR vk_handle;

	/*
	 * This is DIFFERENT from the device's current_frame_index.
	 * A swapchain might have, e.g: 3 frames while the graphics
	 * device only has 2 frames in flight. They are *usually* the same
	 * but not always!
	 */
	u32 current_texture_index;

	u32 texture_count;
	G_TextureKey *textures;
	G_TextureView *views;

	u32 width;
	u32 height;

	VkFormat format;
};

static inline G_TextureKey G_SwapchainCurrentTexture(const G_Swapchain *swapchain)
{
	return swapchain->textures[swapchain->current_texture_index];
}

static inline G_TextureView *G_SwapchainCurrentView(const G_Swapchain *swapchain)
{
	return &swapchain->views[swapchain->current_texture_index];
}

#endif // GRAPHICS_SWAPCHAIN_H
