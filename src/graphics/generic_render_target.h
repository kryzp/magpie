#ifndef VK_GENERIC_RENDER_TARGET_H_
#define VK_GENERIC_RENDER_TARGET_H_

#include <vulkan/vulkan.h>

#include "../common.h"

#include "../container/array.h"

#include "render_info_builder.h"
#include "texture.h"

namespace llt
{
	class VulkanBackend;
	class RenderInfoBuilder;

	enum RenderTargetType
	{
		RENDER_TARGET_TYPE_NONE = 0,
		RENDER_TARGET_TYPE_TEXTURE,
		RENDER_TARGET_TYPE_BACKBUFFER
	};

	class GenericRenderTarget
	{
	public:
		GenericRenderTarget();
		GenericRenderTarget(uint32_t width, uint32_t height);
		virtual ~GenericRenderTarget();

		virtual void cleanUp() = 0;

		virtual RenderInfoBuilder* getRenderInfo();

		virtual void beginRender(VkCommandBuffer cmdBuffer) = 0;
		virtual void endRender(VkCommandBuffer cmdBuffer) = 0;

		uint64_t getAttachmentCount() const { return m_renderInfo.getColourAttachmentCount(); }

		virtual const Texture* getAttachment(int idx) const = 0;
		virtual const Texture* getDepthAttachment() const = 0;

		virtual VkSampleCountFlagBits getMSAA() const = 0;

		void toggleClear(int idx, bool clear)
		{
			m_renderInfo.getColourAttachment(idx).loadOp = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
		}

		void toggleClear(bool clear)
		{
			for (int i = 0; i < getAttachmentCount(); i++) {
				toggleClear(i, clear);
			}
		}

		virtual void setClearColour(int idx, const Colour& colour) = 0;
		virtual void setDepthStencilClear(float depth, uint32_t stencil) = 0;

		void setClearColours(const Colour& colour)
		{
			for (int i = 0; i < getAttachmentCount(); i++) {
				setClearColour(i, colour);
			}
		}

		uint32_t getWidth() const;
		uint32_t getHeight() const;
		RenderTargetType getType() const;

	protected:
		uint32_t m_width;
		uint32_t m_height;
		RenderTargetType m_type;

		RenderInfoBuilder m_renderInfo;
	};
}

#endif // VK_GENERIC_RENDER_TARGET_H_
