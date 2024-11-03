#include "render_pass_builder.h"
#include "backend.h"
#include "util.h"
#include "backbuffer.h"

using namespace llt;

RenderPassBuilder::RenderPassBuilder()
	: m_clearColours()
	, m_renderPass(VK_NULL_HANDLE)
	, m_attachmentDescriptions()
	, m_colourAttachments{}
	, m_colourAttachmentResolves{}
	, m_depthAttachmentRef()
	, m_colourAttachmentCount(0)
	, m_attachmentCount(0)
	, m_width(0)
	, m_height(0)
	, m_framebuffers()
	, m_backbuffer(nullptr)
{
}

RenderPassBuilder::~RenderPassBuilder()
{
	cleanUp();
}

void RenderPassBuilder::createColourAttachment(int idx, VkFormat format, VkSampleCountFlagBits samples, VkAttachmentLoadOp loadOp, VkImageLayout finalLayout, bool isResolve)
{
	m_attachmentDescriptions[idx].format = format;
	m_attachmentDescriptions[idx].samples = samples;
	m_attachmentDescriptions[idx].loadOp = loadOp;
	m_attachmentDescriptions[idx].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	m_attachmentDescriptions[idx].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	m_attachmentDescriptions[idx].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	m_attachmentDescriptions[idx].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	m_attachmentDescriptions[idx].finalLayout = finalLayout;

	VkAttachmentReference swapChainColourAttachmentRef = {};
	swapChainColourAttachmentRef.attachment = idx;
	swapChainColourAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	if (!isResolve) {
		m_colourAttachments.pushBack(swapChainColourAttachmentRef);
		m_colourAttachmentCount++;
	} else {
		m_colourAttachmentResolves.pushBack(swapChainColourAttachmentRef);
	}

	m_attachmentCount++;
}

void RenderPassBuilder::createDepthAttachment(int idx, VkSampleCountFlagBits samples)
{
	m_attachmentDescriptions[idx].format = vkutil::findDepthFormat(g_vulkanBackend->physicalData.device);
	m_attachmentDescriptions[idx].samples = samples;
	m_attachmentDescriptions[idx].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	m_attachmentDescriptions[idx].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	m_attachmentDescriptions[idx].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	m_attachmentDescriptions[idx].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	m_attachmentDescriptions[idx].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	if (m_colourAttachmentCount == 0) { // assume we want to read the depth stencil buffer instead since we are a dedicated depth buffer
		m_attachmentDescriptions[idx].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		m_attachmentDescriptions[idx].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	} else {
		m_attachmentDescriptions[idx].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	}

	m_depthAttachmentRef.attachment = idx;
	m_depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	m_attachmentCount++;
}

void RenderPassBuilder::createRenderPass(uint64_t nFramebuffers, uint64_t attachmentCount, VkImageView* attachments, VkImageView* backbufferAttachments, uint64_t backbufferAttachmentIdx)
{
	destroyRenderPass();
	destroyFBOs();

	VkSubpassDescription swapChainSubPass = {};
	swapChainSubPass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	swapChainSubPass.colorAttachmentCount = m_colourAttachmentCount;
	swapChainSubPass.pColorAttachments = m_colourAttachments.data();
	swapChainSubPass.pResolveAttachments = m_colourAttachmentResolves.data();
	swapChainSubPass.pDepthStencilAttachment = &m_depthAttachmentRef;

	VkSubpassDependency subPassDependency = {};
	subPassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	subPassDependency.dstSubpass = 0;
	subPassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	subPassDependency.srcAccessMask = 0;
	subPassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	subPassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo renderPassCreateInfo = {};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.pAttachments = m_attachmentDescriptions.data();
	renderPassCreateInfo.attachmentCount = m_attachmentCount;
	renderPassCreateInfo.pSubpasses = &swapChainSubPass;
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pDependencies = &subPassDependency;
	renderPassCreateInfo.dependencyCount = 1;

	if (VkResult result = vkCreateRenderPass(g_vulkanBackend->device, &renderPassCreateInfo, nullptr, &m_renderPass); result != VK_SUCCESS) {
		LLT_ERROR("[VULKAN:RENDERPASSBUILDER|DEBUG] Failed to create render pass: %d", result);
	}

	m_framebuffers.resize(nFramebuffers);

	for (uint64_t n = 0; n < nFramebuffers; n++)
    {
		if (backbufferAttachments != nullptr) {
			attachments[backbufferAttachmentIdx] = backbufferAttachments[n];
		}

		VkFramebufferCreateInfo framebufferCreateInfo = {};
		framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferCreateInfo.renderPass = m_renderPass;
		framebufferCreateInfo.pAttachments = attachments;
		framebufferCreateInfo.attachmentCount = attachmentCount;
		framebufferCreateInfo.width = m_width;
		framebufferCreateInfo.height = m_height;
		framebufferCreateInfo.layers = 1;

		if (VkResult result = vkCreateFramebuffer(g_vulkanBackend->device, &framebufferCreateInfo, nullptr, &m_framebuffers[n]); result != VK_SUCCESS) {
			LLT_ERROR("[VULKAN:RENDERPASSBUILDER|DEBUG] Failed to create framebuffer for render pass: %d", result);
		}
	}

	LLT_LOG("[VULKAN:RENDERPASSBUILDER] Created render pass!");
}

void RenderPassBuilder::cleanUp()
{
	destroyRenderPass();
	destroyFBOs();

	m_width = 0;
	m_height = 0;
}

void RenderPassBuilder::destroyRenderPass()
{
	if (m_renderPass != VK_NULL_HANDLE)
	{
		vkDestroyRenderPass(g_vulkanBackend->device, m_renderPass, nullptr);
		m_renderPass = VK_NULL_HANDLE;
	}
}

void RenderPassBuilder::cleanUpFramebuffers()
{
	destroyFBOs();

	m_framebuffers.clear();
	m_attachmentDescriptions.clear();
	m_colourAttachments.clear();
	m_colourAttachmentResolves.clear();
	m_depthAttachmentRef = {};
	m_colourAttachmentCount = 0;
	m_attachmentCount = 0;
}

void RenderPassBuilder::destroyFBOs()
{
	for (auto& fbo : m_framebuffers)
	{
		if (fbo != VK_NULL_HANDLE)
		{
			vkDestroyFramebuffer(g_vulkanBackend->device, fbo, nullptr);
			fbo = VK_NULL_HANDLE;
		}
	}
}

VkRenderPassBeginInfo RenderPassBuilder::buildBeginInfo() const
{
	uint32_t framebufferIndex = 0;

	if (m_backbuffer) {
		framebufferIndex = m_backbuffer->getCurrentTextureIdx();
	}

	VkRenderPassBeginInfo renderPassBeginInfo = {};

	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = m_renderPass;
	renderPassBeginInfo.framebuffer = m_framebuffers[framebufferIndex % m_framebuffers.size()];
	renderPassBeginInfo.renderArea.offset = { 0, 0 };
	renderPassBeginInfo.renderArea.extent = { m_width, m_height };
	renderPassBeginInfo.pClearValues = m_clearColours.data();
	renderPassBeginInfo.clearValueCount = m_clearColours.size();

	return renderPassBeginInfo;
}

void RenderPassBuilder::makeBackbuffer(Backbuffer* backbuffer)
{
	m_backbuffer = backbuffer;
}

void RenderPassBuilder::setClearColour(int idx, VkClearValue value)
{
	m_clearColours[idx] = value;
}

void RenderPassBuilder::setClearValues(const Vector<VkClearValue>& values)
{
	m_clearColours = values;
}

VkRenderPass RenderPassBuilder::getRenderPass()
{
	return m_renderPass;
}

void RenderPassBuilder::setDimensions(uint32_t width, uint32_t height)
{
	m_width = width;
	m_height = height;
}

int RenderPassBuilder::getColourAttachmentCount() const
{
	return m_colourAttachmentCount;
}

int RenderPassBuilder::getAttachmentCount() const
{
	return m_attachmentCount;
}

uint32_t RenderPassBuilder::getWidth() const
{
	return m_width;
}

uint32_t RenderPassBuilder::getHeight() const
{
	return m_height;
}
