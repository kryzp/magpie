#pragma once

#include <array>
#include <vector>

#include <Volk/volk.h>

#include "graphics_core.h"

namespace mgp
{
	class GraphicsCore;
	class PlatformCore;

	class Image;
	class ImageView;

	class Swapchain
	{
	public:
		Swapchain(GraphicsCore *gfx, PlatformCore *platform);
		~Swapchain();

		void acquireNextImage();

		Image *getCurrentSwapchainImage();
		ImageView *getCurrentView();

		unsigned getCurrentSwapchainImageIndex() const;

		void onWindowResize(int width, int height);

		VkSemaphoreSubmitInfo getRenderFinishedSemaphoreSubmitInfo() const;
		VkSemaphoreSubmitInfo getImageAvailableSemaphoreSubmitInfo() const;

		const VkSwapchainKHR &getHandle() const;
		const VkFormat &getSwapchainImageFormat() const;

		void rebuildSwapchain();

		uint32_t getWidth() const;
		uint32_t getHeight() const;

	private:
		GraphicsCore *m_gfx;
		PlatformCore *m_platform;

		void destroy();

		void createSwapchain();
		void createSwapchainSyncObjects();

		std::array<VkSemaphore, gfx_constants::FRAMES_IN_FLIGHT> m_renderFinishedSemaphores;
		std::array<VkSemaphore, gfx_constants::FRAMES_IN_FLIGHT> m_imageAvailableSemaphores;

		VkSwapchainKHR m_swapchain;

		std::vector<Image> m_swapchainImages;
		std::vector<ImageView *> m_swapchainImageViews;

		VkFormat m_swapchainImageFormat;

		unsigned m_currSwapchainImageIdx;

		unsigned m_width;
		unsigned m_height;
	};
}
