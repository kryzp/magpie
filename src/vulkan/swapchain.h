#pragma once

#include <array>
#include <vector>

#include "third_party/volk.h"

#include "math/colour.h"

#include "core.h"
#include "image.h"
#include "image_view.h"
#include "queue.h"

namespace mgp
{
	class Platform;
	class CommandBuffer;

	class Swapchain
	{
	public:
		Swapchain(VulkanCore *core, const Platform *platform);
		~Swapchain();

		void acquireNextImage();

		Image &getColourAttachment();
		Image &getDepthAttachment();

		ImageInfo &getCurrentSwapchainImage();
		ImageView *getCurrentSwapchainImageView() const;

		unsigned getCurrentSwapchainImageIndex() const;

		void onWindowResize(int width, int height);

		VkSemaphoreSubmitInfo getRenderFinishedSemaphoreSubmitInfo() const;
		VkSemaphoreSubmitInfo getImageAvailableSemaphoreSubmitInfo() const;

		const VkSwapchainKHR &getHandle() const;
		const VkFormat &getSwapchainImageFormat() const;

		void rebuildSwapchain();

		unsigned getWidth() const;
		unsigned getHeight() const;

	private:
		void destroy();

		void createSwapchain();
		void createSwapchainImageViews();
		void createSwapchainSyncObjects();

		void createColourResources();
		void createDepthResources();

		std::array<VkSemaphore, Queue::FRAMES_IN_FLIGHT> m_renderFinishedSemaphores;
		std::array<VkSemaphore, Queue::FRAMES_IN_FLIGHT> m_imageAvailableSemaphores;

		VkSwapchainKHR m_swapchain;

		std::vector<ImageInfo> m_swapchainImages;
		std::vector<ImageView *> m_swapchainImageViews;

		VkFormat m_swapchainImageFormat;

		unsigned m_currSwapchainImageIdx;

		Image m_colour;
		Image m_depth;

		unsigned m_width;
		unsigned m_height;

		VulkanCore *m_core;
		const Platform *m_platform;
	};
}
