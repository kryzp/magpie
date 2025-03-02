#ifndef VK_RENDER_TARGET_MGR_H_
#define VK_RENDER_TARGET_MGR_H_

#include "../third_party/volk.h"

#include "../container/vector.h"
#include "../container/hash_map.h"
#include "../container/string.h"

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

        RenderTarget* get(const String& name);

		RenderTarget* createTarget(
            const String& name,
            uint32_t width, uint32_t height,
            const Vector<VkFormat>& attachments,
            VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT
        );

    private:
        HashMap<String, RenderTarget*> m_targets;
    };

    extern RenderTargetMgr* g_renderTargetManager;
}

#endif // VK_RENDER_TARGET_MGR_H_
