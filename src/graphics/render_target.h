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
		void createOnlyDepth();

		void cleanUp() override;
		
		void setClearColour(int idx, const Colour& colour) override;
		void setDepthStencilClear(float depth, uint32_t stencil) override;

		const Texture* getAttachment(int idx) const override;
		const Texture* getDepthAttachment() const override;

		void setAttachment(int idx, Texture* texture);

		VkSampleCountFlagBits getMSAA() const override;

	private:
		void createDepthResources(int idx);

		Array<Texture*, mgc::MAX_RENDER_TARGET_ATTACHMENTS> m_attachments;
		Texture m_depth;
	};
}

#endif // VK_RENDER_TARGET_H_
