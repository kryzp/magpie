#ifndef GRAPHICS_SWAPCHAIN_H
#define GRAPHICS_SWAPCHAIN_H

#include "core/types.h"
#include "graphics/resource_key.h"
#include "graphics/texture.h"

#include "container/vector.h"

#include <volk/volk.h>

#define GRAPHICS_FRAMES_IN_FLIGHT 3

namespace gfx {
class Device;

struct SwapchainFrame {
	ResourceKey texture_key;
	TextureView texture_view;
	VkSemaphore render_finished_semaphore = VK_NULL_HANDLE; // Signaled when the OS lets us present.
};

class Swapchain {
	friend class Device;

public:
	explicit Swapchain(Device &dev);
	~Swapchain();

	Swapchain(const Swapchain &) = delete;
	Swapchain &operator=(const Swapchain &) = delete;

	const SwapchainFrame &GetCurrentFrame() const;

	unsigned GetWidth() const;
	unsigned GetHeight() const;

	VkFormat GetFormat() const;

	bool NeedsManualSRGBEncode() const;

private:
	Device *device;

	VkSwapchainKHR vk_handle;

	/*
	 * This is DIFFERENT from the device's current_frame_index.
	 * A swapchain might have, e.g: 3 frames while the graphics
	 * device only has 2 frames in flight. They are *usually* the same
	 * but not always!
	 */
	unsigned current_frame_index;

	Vector<SwapchainFrame> frames;

	unsigned width;
	unsigned height;

	VkFormat format;

	bool needs_manual_srgb_encode;
};

}

#endif // GRAPHICS_SWAPCHAIN_H
