#ifndef GRAPHICS_SWAPCHAIN_H
#define GRAPHICS_SWAPCHAIN_H

#define G_FRAMES_IN_FLIGHT 3

typedef struct G_SwapchainFrame G_SwapchainFrame;
struct G_SwapchainFrame
{
	G_TextureKey texture_key;
	G_TextureView texture_view;
	
	VkSemaphore render_finished_semaphore; // Signaled when the OS let's us present.
};

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
	u32 current_frame_index;

	u32 frame_count;
	G_SwapchainFrame *frames;

	u32 width;
	u32 height;

	VkFormat format;
};

#endif // GRAPHICS_SWAPCHAIN_H
