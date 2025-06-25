#include "swapchain.h"

#include "core/common.h"

#include "graphics_core.h"
#include "toolbox.h"
#include "command_buffer.h"
#include "queue.h"
#include "image.h"
#include "image_view.h"

using namespace mgp;

Swapchain::Swapchain(GraphicsCore *gfx, PlatformCore *platform)
	: m_renderFinishedSemaphores()
	, m_imageAvailableSemaphores()
	, m_swapchain()
	, m_swapchainImages()
	, m_swapchainImageFormat()
	, m_currSwapchainImageIdx()
	, m_width(0)
	, m_height(0)
	, m_gfx(gfx)
	, m_platform(platform)
{
	createSwapchain();
}

Swapchain::~Swapchain()
{
	destroy();
}

void Swapchain::destroy()
{
	for (auto &i : m_swapchainImageViews)
	{
		delete i;
	}

	for (int i = 0; i < gfx_constants::FRAMES_IN_FLIGHT; i++)
	{
		vkDestroySemaphore(m_gfx->getLogicalDevice(), m_renderFinishedSemaphores[i], nullptr);
		vkDestroySemaphore(m_gfx->getLogicalDevice(), m_imageAvailableSemaphores[i], nullptr);
	}

	vkDestroySwapchainKHR(m_gfx->getLogicalDevice(), m_swapchain, nullptr);
}

void Swapchain::acquireNextImage()
{
	// try to get the next image
	// if it is deemed out of date then rebuild the swap chain
	// otherwise this is an unknown issue and throw an error

	VkAcquireNextImageInfoKHR info = {};
	info.sType = VK_STRUCTURE_TYPE_ACQUIRE_NEXT_IMAGE_INFO_KHR;
	info.swapchain = m_swapchain;
	info.timeout = UINT64_MAX;
	info.semaphore = getImageAvailableSemaphoreSubmitInfo().semaphore;
	info.fence = VK_NULL_HANDLE;
	info.deviceMask = 1;

	if (VkResult result = vkAcquireNextImage2KHR(m_gfx->getLogicalDevice(), &info, &m_currSwapchainImageIdx); result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		rebuildSwapchain();
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		mgp_ERROR("Failed to acquire next image in swap chain: %d", result);
	}
}

Image *Swapchain::getCurrentSwapchainImage()
{
	return &m_swapchainImages[m_currSwapchainImageIdx];
}

ImageView *Swapchain::getCurrentView()
{
	return m_swapchainImageViews[m_currSwapchainImageIdx];
}

unsigned Swapchain::getCurrentSwapchainImageIndex() const
{
	return m_currSwapchainImageIdx;
}

void Swapchain::createSwapchain()
{
	SwapchainSupportDetails details = vk_toolbox::querySwapChainSupport(m_gfx->getPhysicalDevice(), m_gfx->getSurface().getHandle());

	// get the surface settings
	auto surfaceFormat = vk_toolbox::chooseSwapSurfaceFormat(details.surfaceFormats);
	auto presentMode = vk_toolbox::chooseSwapPresentMode(details.presentModes, true); // temporary, we just enable vsync regardless of config
	auto extent = vk_toolbox::chooseSwapExtent(m_platform, details.capabilities);

	// set size
	m_width = extent.width;
	m_height = extent.height;

	// set format
	m_swapchainImageFormat = surfaceFormat.format;

	// make sure our image count can't go above the maximum image count
	// but is as high as possible.
	uint32_t imageCount = details.capabilities.minImageCount + 1;

	if (details.capabilities.maxImageCount > 0 && imageCount > details.capabilities.maxImageCount)
	{
		imageCount = details.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = m_gfx->getSurface().getHandle();
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.queueFamilyIndexCount = 0;
	createInfo.pQueueFamilyIndices = nullptr;
	createInfo.preTransform = details.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	// create the swapchain!
	mgp_VK_CHECK(
		vkCreateSwapchainKHR(m_gfx->getLogicalDevice(), &createInfo, nullptr, &m_swapchain),
		"Failed to create swap chain"
	);

	vkGetSwapchainImagesKHR(m_gfx->getLogicalDevice(), m_swapchain, &imageCount, nullptr);

	if (!imageCount)
	{
		mgp_ERROR("Failed to find any images in swap chain!");
	}

	m_swapchainImages.resize(imageCount);

	// now that we know for sure that we must have swap chain images (and the number of them)
	// actually get the swap chain images array
	std::vector<VkImage> images(imageCount);
	vkGetSwapchainImagesKHR(m_gfx->getLogicalDevice(), m_swapchain, &imageCount, images.data());

	for (int i = 0; i < images.size(); i++)
	{
		Image &imageInfo = m_swapchainImages[i];

		imageInfo.wrapAround(
			m_gfx,
			images[i],
			VK_IMAGE_LAYOUT_UNDEFINED,
			m_width, m_height, 1,
			m_swapchainImageFormat,
			VK_IMAGE_VIEW_TYPE_2D,
			VK_IMAGE_TILING_OPTIMAL,
			1,
			VK_SAMPLE_COUNT_1_BIT,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT
		);

		m_swapchainImageViews.push_back(m_gfx->createImageView(&imageInfo, imageInfo.getLayerCount(), 0, 0));
	}

	createSwapchainSyncObjects();

	mgp_LOG("Created the swap chain!");
}

void Swapchain::createSwapchainSyncObjects()
{
	VkSemaphoreCreateInfo semaphoreCreateInfo = {};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	// go through all our frames in flight and create the semaphores for each frame
	for (int i = 0; i < gfx_constants::FRAMES_IN_FLIGHT; i++)
	{
		mgp_VK_CHECK(
			vkCreateSemaphore(m_gfx->getLogicalDevice(), &semaphoreCreateInfo, nullptr, &m_imageAvailableSemaphores[i]),
			"Failed to create image available semaphore"
		);

		mgp_VK_CHECK(
			vkCreateSemaphore(m_gfx->getLogicalDevice(), &semaphoreCreateInfo, nullptr, &m_renderFinishedSemaphores[i]),
			"Failed to create render finished semaphore"
		);
	}
}

void Swapchain::onWindowResize(int width, int height)
{
	if (width == 0 || height == 0)
		return;

	if (m_width == width && m_height == height)
		return;

	m_width = width;
	m_height = height;

	// we have to rebuild the swapchain to reflect this
	// also get the next image in the queue to move away from our out-of-date current one
	rebuildSwapchain();
//	acquireNextImage(); // todo: might not be necessary
}

void Swapchain::rebuildSwapchain()
{
	m_gfx->waitIdle();

	destroy();
	createSwapchain();
}

unsigned Swapchain::getWidth() const
{
	return m_width;
}

unsigned Swapchain::getHeight() const
{
	return m_height;
}

VkSemaphoreSubmitInfo Swapchain::getRenderFinishedSemaphoreSubmitInfo() const
{
	VkSemaphoreSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
	submitInfo.semaphore = m_renderFinishedSemaphores[m_gfx->getCurrentFrameIndex()];
	submitInfo.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

	return submitInfo;
}

VkSemaphoreSubmitInfo Swapchain::getImageAvailableSemaphoreSubmitInfo() const
{
	VkSemaphoreSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
	submitInfo.semaphore = m_imageAvailableSemaphores[m_gfx->getCurrentFrameIndex()];
	submitInfo.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

	return submitInfo;
}

const VkSwapchainKHR &Swapchain::getHandle() const
{
	return m_swapchain;
}

const VkFormat &Swapchain::getSwapchainImageFormat() const
{
	return m_swapchainImageFormat;
}
