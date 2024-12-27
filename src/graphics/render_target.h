#ifndef VK_RENDER_TARGET_H_
#define VK_RENDER_TARGET_H_

#include "generic_render_target.h"

namespace llt
{
	class RenderTarget : public GenericRenderTarget
	{
	public:
		RenderTarget();
		RenderTarget(uint32_t width, uint32_t height);
		~RenderTarget() override;

		void create();

		void beginGraphics(VkCommandBuffer cmdBuffer) override;
		void endGraphics(VkCommandBuffer cmdBuffer) override;

		void cleanUp() override;
		
		void setClearColour(int idx, const Colour& colour) override;
		void setDepthStencilClear(float depth, uint32_t stencil) override;

		Texture* getAttachment(int idx) override;
		Texture* getDepthAttachment() override;

		void addAttachment(Texture* texture);

		VkSampleCountFlagBits getMSAA() const override;
		void setMSAA(VkSampleCountFlagBits samples);

	private:
		void createDepthResources();

		Vector<Texture*> m_attachments;
		Vector<Texture*> m_resolveAttachments;

		Texture m_depth;
		Texture m_resolveDepth;

		VkSampleCountFlagBits m_samples;
	};
}

#endif // VK_RENDER_TARGET_H_
