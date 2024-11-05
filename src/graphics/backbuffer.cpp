#include "backbuffer.h"
#include "util.h"
#include "backend.h"
#include "colour.h"

#include "../system_backend.h"

using namespace llt;

Backbuffer::Backbuffer()
	: GenericRenderTarget()
	, m_renderPassBuilder()
    , m_swapChain(VK_NULL_HANDLE)
    , m_swapChainImages()
    , m_swapChainImageViews()
    , m_surface(VK_NULL_HANDLE)
    , m_currSwapChainImageIdx(0)
	, m_colour()
	, m_depth()
	, m_renderFinishedSemaphores()
	, m_imageAvailableSemaphores()
{
	m_type = RENDER_TARGET_TYPE_BACKBUFFER;
}

Backbuffer::~Backbuffer()
{
	cleanUp();
}

void Backbuffer::create()
{
	createSwapChain();
	acquireNextImage();
}

void Backbuffer::createSurface()
{
    if (bool result = g_systemBackend->vkCreateSurface(g_vulkanBackend->vulkanInstance, &m_surface); !result) {
        LLT_ERROR("[VULKAN:BACKBUFFER|DEBUG] Failed to create surface: %d", result);
    }
}

void Backbuffer::createColourResources()
{
	// build the colour resource
	m_colour.initSize(m_width, m_height);
	m_colour.initMetadata(g_vulkanBackend->swapChainImageFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_VIEW_TYPE_2D);
	m_colour.initSampleCount(g_vulkanBackend->msaaSamples);
	m_colour.initTransient(true);
	m_colour.createInternalResources();

	// add the colour resources to our render pass builder
	m_renderPassBuilder.createColourAttachment(0,
		g_vulkanBackend->swapChainImageFormat,
		g_vulkanBackend->msaaSamples,
		VK_ATTACHMENT_LOAD_OP_CLEAR,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		false
	);

	// add the resolve colour component due to MSAA
	m_renderPassBuilder.createColourAttachment(2,
		g_vulkanBackend->swapChainImageFormat,
		VK_SAMPLE_COUNT_1_BIT,
		VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		true
	);

	LLT_LOG("[VULKAN:BACKBUFFER] Created colour resources!");
}

void Backbuffer::createDepthResources()
{
    VkFormat format = vkutil::findDepthFormat(g_vulkanBackend->physicalData.device);

	// create the depth component
	m_depth.initSize(m_width, m_height);
	m_depth.initMetadata(format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_VIEW_TYPE_2D);
	m_depth.initSampleCount(g_vulkanBackend->msaaSamples);
	
	m_depth.createInternalResources();

	m_depth.transitionLayout(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

	// add the component to our render pass builder
	m_renderPassBuilder.createDepthAttachment(1, g_vulkanBackend->msaaSamples);

    LLT_LOG("[VULKAN:BACKBUFFER] Created depth resources!");
}

void Backbuffer::cleanUp()
{
	// clean up the render pass builder
	m_renderPassBuilder.cleanUp();

	// if our surface exists, destroy it
	if (m_surface != VK_NULL_HANDLE)
	{
		vkDestroySurfaceKHR(g_vulkanBackend->vulkanInstance, m_surface, nullptr);
		m_surface = VK_NULL_HANDLE;
	}
}

void Backbuffer::cleanUpTextures()
{
	m_depth.cleanUp();
	m_colour.cleanUp();
}

void Backbuffer::cleanUpSwapChain()
{
	cleanUpTextures();

	// clean up the render targets we have
	m_renderPassBuilder.cleanUpFramebuffers();

	// destroy all of our semaphores
	for (int i = 0; i < mgc::FRAMES_IN_FLIGHT; i++) {
		vkDestroySemaphore(g_vulkanBackend->device, m_renderFinishedSemaphores[i], nullptr);
		vkDestroySemaphore(g_vulkanBackend->device, m_imageAvailableSemaphores[i], nullptr);
	}

	m_renderFinishedSemaphores.clear();
	m_imageAvailableSemaphores.clear();

	// destroy all image views for our swapchain
    for (auto& view : m_swapChainImageViews) {
        vkDestroyImageView(g_vulkanBackend->device, view, nullptr);
    }

	// finally destroy the actual swapchain
    vkDestroySwapchainKHR(g_vulkanBackend->device, m_swapChain, nullptr);
}

RenderPassBuilder* Backbuffer::getRenderPassBuilder()
{
	return &m_renderPassBuilder;
}

void Backbuffer::acquireNextImage()
{
	// try to get the next image
	// if it is deemed out of date then rebuild the swap chain
	// otherwise this is an unknown issue and throw an error
    if (VkResult result = vkAcquireNextImageKHR(g_vulkanBackend->device, m_swapChain, UINT64_MAX, getImageAvailableSemaphore(), VK_NULL_HANDLE, &m_currSwapChainImageIdx); result == VK_ERROR_OUT_OF_DATE_KHR) {
        rebuildSwapChain();
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        LLT_ERROR("[VULKAN:BACKBUFFER|DEBUG] Failed to acquire next image in swap chain: %d", result);
    }
}

void Backbuffer::swapBuffers()
{
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &getRenderFinishedSemaphore();
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &m_swapChain;
    presentInfo.pImageIndices = &m_currSwapChainImageIdx;
    presentInfo.pResults = nullptr;

	// queue a new present info
	// if it returns as being out of date or suboptimal
	// rebuild the swap chain
    if (VkResult result = vkQueuePresentKHR(g_vulkanBackend->graphicsQueue.getQueue(), &presentInfo); result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
		rebuildSwapChain();
	} else if (result != VK_SUCCESS) {
        LLT_ERROR("[VULKAN:BACKBUFFER|DEBUG] Failed to present swap chain image: %d", result);
    }
}

const Texture* Backbuffer::getAttachment(int idx) const
{
	return &m_colour;
}

const Texture* Backbuffer::getDepthAttachment() const
{
	return &m_depth;
}

void Backbuffer::createSwapChain()
{
    SwapChainSupportDetails details = vkutil::querySwapChainSupport(g_vulkanBackend->physicalData.device, m_surface);

	// get the surface settings
    auto surfaceFormat = vkutil::chooseSwapSurfaceFormat(details.surfaceFormats);
    auto presentMode = vkutil::chooseSwapPresentMode(details.presentModes);
    auto extent = vkutil::chooseSwapExtent(details.capabilities);

	// make sure our image count can't go above the maximum image count
	// but is as high as possible.
    uint32_t imageCount = details.capabilities.minImageCount + 1;
    if (details.capabilities.maxImageCount > 0 && imageCount > details.capabilities.maxImageCount) {
        imageCount = details.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = m_surface;
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
    if (VkResult result = vkCreateSwapchainKHR(g_vulkanBackend->device, &createInfo, nullptr, &m_swapChain); result != VK_SUCCESS) {
        LLT_ERROR("[VULKAN:BACKBUFFER|DEBUG] Failed to create swap chain: %d", result);
    }

	// get the swapchain images
    vkGetSwapchainImagesKHR(g_vulkanBackend->device, m_swapChain, &imageCount, nullptr);

	// if we weren't able to locate any throw an error
    if (!imageCount) {
        LLT_ERROR("[VULKAN:BACKBUFFER|DEBUG] Failed to find any images in swap chain!");
    }

    m_swapChainImages.resize(imageCount);

	// now that we know for sure that we must have swap chain images (and the number of them)
	// actually get the swap chain images array
    VkImage images[imageCount];
    vkGetSwapchainImagesKHR(g_vulkanBackend->device, m_swapChain, &imageCount, images);

	// store this info
    for (int i = 0; i < imageCount; i++) {
        m_swapChainImages[i] = images[i];
    }

	g_vulkanBackend->swapChainImageFormat = surfaceFormat.format;

	createSwapChainSyncObjects();
	createSwapChainImageViews();

	VkExtent2D ext = vkutil::chooseSwapExtent(details.capabilities);

	m_width = ext.width;
	m_height = ext.height;

	// tell our render pass builder to treat us differently
	// because we are a backbuffer and not a regular render target
	m_renderPassBuilder.makeBackbuffer(this);

	// set the default clear values for depth and stencil
	// white for depth and 0 for stencil
	Vector<VkClearValue> values(2);
	Colour::black().getPremultiplied().exportToFloat(values[0].color.float32);
	values[1].depthStencil = { 1.0f, 0 };
	m_renderPassBuilder.setClearValues(values);

	// set our dimensions and init
	m_renderPassBuilder.setDimensions(m_width, m_height);

	// create resources
	createDepthResources();
	createColourResources();

	VkImageView attachments[] = {
		m_colour.getImageView(),
		m_depth.getImageView(),
		m_swapChainImageViews[0]
	};

	// create the render pass with our attachments
	m_renderPassBuilder.createRenderPass(
		m_swapChainImageViews.size(),
		LLT_ARRAY_LENGTH(attachments), attachments,
		m_swapChainImageViews.data(), 2
	);

    LLT_LOG("[VULKAN:BACKBUFFER] Created the swap chain!");
}

void Backbuffer::createSwapChainSyncObjects()
{
	VkSemaphoreCreateInfo semaphoreCreateInfo = {};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	// go through all our frames in flight and create the semaphores for each frame
	for (int i = 0; i < mgc::FRAMES_IN_FLIGHT; i++)
	{
		if (VkResult result = vkCreateSemaphore(g_vulkanBackend->device, &semaphoreCreateInfo, nullptr, &m_imageAvailableSemaphores[i]); result != VK_SUCCESS) {
			LLT_ERROR("[VULKAN:BACKBUFFER|DEBUG] Failed to create image available semaphore: %d", result);
		}

		if (VkResult result = vkCreateSemaphore(g_vulkanBackend->device, &semaphoreCreateInfo, nullptr, &m_renderFinishedSemaphores[i]); result != VK_SUCCESS) {
			LLT_ERROR("[VULKAN:BACKBUFFER|DEBUG] Failed to create render finished semaphore: %d", result);
		}
	}

	LLT_LOG("[VULKAN:BACKBUFFER] Created sync objects!");
}

void Backbuffer::createSwapChainImageViews()
{
    m_swapChainImageViews.resize(m_swapChainImages.size());

	// create an image view for each swap chian image
    for (uint64_t i = 0; i < m_swapChainImages.size(); i++)
    {
        VkImageViewCreateInfo viewInfo = {};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = m_swapChainImages[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = g_vulkanBackend->swapChainImageFormat;

        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.layerCount = 1;

        viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        if (VkResult result = vkCreateImageView(g_vulkanBackend->device, &viewInfo, nullptr, &m_swapChainImageViews[i]); result != VK_SUCCESS) {
            LLT_ERROR("[VULKAN:BACKBUFFER|DEBUG] Failed to create texture image view: %d", result);
        }
    }

    LLT_LOG("[VULKAN:BACKBUFFER] Created swap chain image views!");
}

void Backbuffer::onWindowResize(int width, int height)
{
	// invalid size
	if (width == 0 || height == 0) {
		return;
	}

	// if we somehow got a resize call but didn't actually change in size then just exit early
	if (m_width == width && m_height == height) {
		return;
	}

	// update to our new dimensions
	m_width = width;
	m_height = height;

	// we have to rebuild the swapchain to reflect this
	// also get the next image in the queue to move away from our out-of-date current one
	rebuildSwapChain();
	acquireNextImage();
}

void Backbuffer::rebuildSwapChain()
{
	// wait until we are synced-up with the gpu
	g_vulkanBackend->syncStall();

	// create a new swapchain
	cleanUpSwapChain();
	createSwapChain();

	VkImageView attachments[] = {
		m_colour.getImageView(),
		m_depth.getImageView(),
		m_swapChainImageViews[0]
	};

	m_renderPassBuilder.setDimensions(m_width, m_height);

	m_renderPassBuilder.createRenderPass(
		m_swapChainImageViews.size(),
		LLT_ARRAY_LENGTH(attachments), attachments,
		m_swapChainImageViews.data(), 2
	);
}

void Backbuffer::setClearColour(int idx, const Colour& colour)
{
	VkClearValue value = {};
	colour.getPremultiplied().exportToFloat(value.color.float32);
	m_renderPassBuilder.setClearColour(0, value);
}

void Backbuffer::setDepthStencilClear(float depth, uint32_t stencil)
{
	VkClearValue value = {};
	value.depthStencil = { depth, stencil };
	m_renderPassBuilder.setClearColour(1, value);
}

VkSurfaceKHR Backbuffer::getSurface() const
{
	return m_surface;
}

int Backbuffer::getCurrentTextureIdx() const
{
	return m_currSwapChainImageIdx;
}

int Backbuffer::getMSAA() const
{
	return g_vulkanBackend->msaaSamples;
}

const VkSemaphore& Backbuffer::getRenderFinishedSemaphore() const
{
	return m_renderFinishedSemaphores[g_vulkanBackend->getCurrentFrameIdx()];
}

const VkSemaphore& Backbuffer::getImageAvailableSemaphore() const
{
	return m_imageAvailableSemaphores[g_vulkanBackend->getCurrentFrameIdx()];
}
