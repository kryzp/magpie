#ifndef VK_RENDER_TARGET_MGR_H_
#define VK_RENDER_TARGET_MGR_H_

#include <vulkan/vulkan.h>
#include "../container/vector.h"

namespace llt
{
    class VulkanBackend;
    class RenderTarget;
	class Texture;

    class RenderTargetMgr
    {
    public:
        RenderTargetMgr();
        ~RenderTargetMgr();

        // todo: should these should have a string key-hashing system also?
		RenderTarget* createTarget(uint32_t width, uint32_t height, const Vector<VkFormat>& attachments);
		RenderTarget* createDepthTarget(uint32_t width, uint32_t height);

    private:
        Vector<RenderTarget*> m_targets;
    };

    extern RenderTargetMgr* g_renderTargetManager;
}

#endif // VK_RENDER_TARGET_MGR_H_
