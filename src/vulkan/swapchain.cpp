#include "swapchain.h"

#include "core/common.h"

#include "toolbox.h"
#include "command_buffer.h"
#include "sync.h"

using namespace mgp;

Swapchain::Swapchain(VulkanCore *core, const Platform *platform)
	: m_renderFinishedSemaphores()
	, m_imageAvailableSemaphores()
	, m_swapchain()
	, m_swapchainImages()
	, m_swapchainImageViews()
	, m_swapchainImageFormat()
	, m_currSwapchainImageIdx()
	, m_colour()
	, m_depth()
	, m_width(0)
	, m_height(0)
	, m_renderInfo(core)
	, m_core(core)
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
	for (int i = 0; i < Queue::FRAMES_IN_FLIGHT; i++)
	{
		vkDestroySemaphore(m_core->getLogicalDevice(), m_renderFinishedSemaphores[i], nullptr);
		vkDestroySemaphore(m_core->getLogicalDevice(), m_imageAvailableSemaphores[i], nullptr);
	}

	for (auto &view : m_swapchainImageViews)
	{
		delete view;
	}

	vkDestroySwapchainKHR(m_core->getLogicalDevice(), m_swapchain, nullptr);
}

void Swapchain::beginRendering(CommandBuffer &cmd)
{
	VkImageMemoryBarrier2 barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;

	barrier.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;

	barrier.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
	barrier.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

	barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	barrier.image = getCurrentSwapchainImage();

	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	cmd.pipelineBarrier(
		0,
		{}, {}, { barrier }
	);

	cmd.transitionLayout(m_colour, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	cmd.transitionLayout(m_depth, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

	m_renderInfo.getColourAttachment(0).resolveImageView = getCurrentSwapchainImageView()->getHandle();
}

void Swapchain::endRendering(CommandBuffer &cmd)
{
	VkImageMemoryBarrier2 barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;

	barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	barrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
	barrier.dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;

	barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	barrier.image = getCurrentSwapchainImage();

	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	cmd.pipelineBarrier(
		0,
		{}, {}, { barrier }
	);

	cmd.transitionLayout(m_colour, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	cmd.transitionLayout(m_depth, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);
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

	if (VkResult result = vkAcquireNextImage2KHR(m_core->getLogicalDevice(), &info, &m_currSwapchainImageIdx); result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		rebuildSwapchain();
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		MGP_ERROR("Failed to acquire next image in swap chain: %d", result);
	}
}

const Image &Swapchain::getColourAttachment() const
{
	return m_colour;
}

const Image &Swapchain::getDepthAttachment() const
{
	return m_depth;
}

VkImage Swapchain::getCurrentSwapchainImage() const
{
	return m_swapchainImages[m_currSwapchainImageIdx];
}

const ImageView *Swapchain::getCurrentSwapchainImageView() const
{
	return m_swapchainImageViews[m_currSwapchainImageIdx];
}

unsigned Swapchain::getCurrentSwapchainImageIndex() const
{
	return m_currSwapchainImageIdx;
}

void Swapchain::createSwapchain()
{
	SwapchainSupportDetails details = vk_toolbox::querySwapChainSupport(m_core->getPhysicalDevice(), m_core->getSurface().getHandle());

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
	createInfo.surface = m_core->getSurface().getHandle();
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
	MGP_VK_CHECK(
		vkCreateSwapchainKHR(m_core->getLogicalDevice(), &createInfo, nullptr, &m_swapchain),
		"Failed to create swap chain"
	);

	vkGetSwapchainImagesKHR(m_core->getLogicalDevice(), m_swapchain, &imageCount, nullptr);

	if (!imageCount)
	{
		MGP_ERROR("Failed to find any images in swap chain!");
	}

	m_swapchainImages.resize(imageCount);

	// now that we know for sure that we must have swap chain images (and the number of them)
	// actually get the swap chain images array
	std::vector<VkImage> images(imageCount);
	vkGetSwapchainImagesKHR(m_core->getLogicalDevice(), m_swapchain, &imageCount, images.data());

	for (int i = 0; i < images.size(); i++)
	{
		m_swapchainImages[i] = images[i];
	}

	createSwapchainSyncObjects();
	createSwapchainImageViews();

	createDepthResources();
	createColourResources();

	m_renderInfo.setClearColour(0, { { 0.0f, 0.0f, 0.0f, 1.0f } });
	m_renderInfo.setClearDepth({ { 1.0f, 0 } });
	m_renderInfo.setSize(m_width, m_height);
	m_renderInfo.setMSAA(m_core->getMaxMSAASamples());

	MGP_LOG("Created the swap chain!");
}

void Swapchain::createSwapchainImageViews()
{
	m_swapchainImageViews.resize(m_swapchainImages.size());

	// create an image view for each swap chain image
	for (uint64_t i = 0; i < m_swapchainImages.size(); i++)
	{
		VkImageViewCreateInfo viewInfo = {};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = m_swapchainImages[i];
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = m_swapchainImageFormat;

		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.layerCount = 1;

		viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		VkImageView swapchainView = VK_NULL_HANDLE;

		MGP_VK_CHECK(
			vkCreateImageView(m_core->getLogicalDevice(), &viewInfo, nullptr, &swapchainView),
			"Failed to create texture image view"
		);

		m_swapchainImageViews[i] = new ImageView(
			m_core,
			swapchainView,
			VK_IMAGE_VIEW_TYPE_2D,
			m_swapchainImageFormat,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
		);
	}
}

void Swapchain::createSwapchainSyncObjects()
{
	VkSemaphoreCreateInfo semaphoreCreateInfo = {};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	// go through all our frames in flight and create the semaphores for each frame
	for (int i = 0; i < Queue::FRAMES_IN_FLIGHT; i++)
	{
		MGP_VK_CHECK(
			vkCreateSemaphore(m_core->getLogicalDevice(), &semaphoreCreateInfo, nullptr, &m_imageAvailableSemaphores[i]),
			"Failed to create image available semaphore"
		);

		MGP_VK_CHECK(
			vkCreateSemaphore(m_core->getLogicalDevice(), &semaphoreCreateInfo, nullptr, &m_renderFinishedSemaphores[i]),
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
	m_core->deviceWaitIdle();

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

void Swapchain::setClearColour(const Colour &colour)
{
	VkClearValue value = {};
	colour.getPremultiplied().exportToFloat(value.color.float32);

	m_renderInfo.setClearColour(0, value);
}

void Swapchain::setDepthStencilClear(float depth, uint32_t stencil)
{
	VkClearValue value = {};
	value.depthStencil = { depth, stencil };

	m_renderInfo.setClearColour(1, value);
}

VkSemaphoreSubmitInfo Swapchain::getRenderFinishedSemaphoreSubmitInfo() const
{
	VkSemaphoreSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
	submitInfo.semaphore = m_renderFinishedSemaphores[m_core->getCurrentFrameIndex()];
	submitInfo.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

	return submitInfo;
}

VkSemaphoreSubmitInfo Swapchain::getImageAvailableSemaphoreSubmitInfo() const
{
	VkSemaphoreSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
	submitInfo.semaphore = m_imageAvailableSemaphores[m_core->getCurrentFrameIndex()];
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

RenderInfo &Swapchain::getRenderInfo()
{
	return m_renderInfo;
}

const RenderInfo &Swapchain::getRenderInfo() const
{
	return m_renderInfo;
}

void Swapchain::createColourResources()
{
	m_colour.create(
		m_core,
		m_width, m_height, 1,
		m_swapchainImageFormat,
		VK_IMAGE_VIEW_TYPE_2D,
		VK_IMAGE_TILING_OPTIMAL,
		1,
		m_core->getMaxMSAASamples(),
		false,
		false
	);

	m_renderInfo.addColourAttachmentWithResolve(
		VK_ATTACHMENT_LOAD_OP_CLEAR,
		*m_colour.getStandardView(),
		getCurrentSwapchainImageView()->getHandle()
	);
}

void Swapchain::createDepthResources()
{
	VkFormat format = vk_toolbox::findDepthFormat(m_core->getPhysicalDevice());

	m_depth.create(
		m_core,
		m_width, m_height, 1,
		format,
		VK_IMAGE_VIEW_TYPE_2D,
		VK_IMAGE_TILING_OPTIMAL,
		1,
		m_core->getMaxMSAASamples(),
		false,
		false
	);

	m_renderInfo.addDepthAttachment(
		VK_ATTACHMENT_LOAD_OP_CLEAR,
		*m_depth.getStandardView()
	);
}
