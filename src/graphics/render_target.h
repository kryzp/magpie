#ifndef VK_RENDER_TARGET_H_
#define VK_RENDER_TARGET_H_

#include "generic_render_target.h"
#include "../container/deque.h"

namespace llt
{
	class RenderTarget : public GenericRenderTarget
	{
	public:
		RenderTarget(uint32_t width, uint32_t height);
		~RenderTarget() override;

		void beginRendering(CommandBuffer &buffer) override;
		void endRendering(CommandBuffer &buffer) override;

		void cleanUp() override;
		
		void clear();

		void setClearColour(int idx, const Colour &colour) override;
		void setDepthStencilClear(float depth, uint32_t stencil) override;

		Texture *getAttachment(int idx) override;
		Texture *getDepthAttachment() override;

		void addAttachment(Texture *texture, int layer = 0, int mip = 0);

		void createDepthAttachment();
		void setDepthAttachment(Texture *texture);

		VkSampleCountFlagBits getMSAA() const override;
		void setMSAA(VkSampleCountFlagBits samples);

	private:
		Vector<Texture*> m_attachments;
		Vector<Texture*> m_resolveAttachments;

		Vector<VkImageView> m_attachmentViews;

		Deque<VkImageLayout> m_layoutQueue;

		Texture *m_depth;
		Texture *m_resolveDepth;

		VkSampleCountFlagBits m_samples;
	};
}

#endif // VK_RENDER_TARGET_H_
