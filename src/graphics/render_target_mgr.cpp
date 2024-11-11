#include "render_target_mgr.h"
#include "render_target.h"
#include "texture.h"
#include "backbuffer.h"
#include "backend.h"

llt::RenderTargetMgr* llt::g_renderTargetManager = nullptr;

using namespace llt;

RenderTargetMgr::RenderTargetMgr()
{
}

RenderTargetMgr::~RenderTargetMgr()
{
	for (auto& target : m_targets) {
		target->cleanUp();
	}
}

RenderTarget* RenderTargetMgr::createTarget(uint32_t width, uint32_t height, const Vector<VkFormat>& attachments)
{
	RenderTarget* result = new RenderTarget(width, height);

	for (int i = 0; i < attachments.size(); i++)
	{
		Texture* texture = new Texture();

		texture->setSize(width, height);
		texture->setProperties(attachments[i], VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_VIEW_TYPE_2D);
		texture->setSampleCount(VK_SAMPLE_COUNT_1_BIT);

		texture->createInternalResources();

		texture->transitionLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		texture->transitionLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		texture->setParent(result);

		result->setAttachment(i, texture);
	}

	result->create();

	m_targets.pushBack(result);
	return result;
}

RenderTarget* RenderTargetMgr::createDepthTarget(uint32_t width, uint32_t height)
{
	RenderTarget* result = new RenderTarget(width, height);

	result->createOnlyDepth();

	m_targets.pushBack(result);
	return result;
}
