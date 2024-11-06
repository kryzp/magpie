#ifndef VK_GENERIC_RENDER_TARGET_H_
#define VK_GENERIC_RENDER_TARGET_H_

#include <vulkan/vulkan.h>

#include "../common.h"

#include "../container/array.h"

#include "render_pass_builder.h"
#include "texture.h"

namespace llt
{
	class VulkanBackend;
	class RenderPassBuilder;

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

		virtual RenderPassBuilder* getRenderPassBuilder();

		virtual const Texture* getAttachment(int idx) const = 0;
		virtual const Texture* getDepthAttachment() const = 0;

		virtual int getMSAA() const = 0;

		virtual void setClearColour(int idx, const Colour& colour) = 0;
		virtual void setDepthStencilClear(float depth, uint32_t stencil) = 0;

		uint32_t getWidth() const;
		uint32_t getHeight() const;
		RenderTargetType getType() const;

	protected:
		uint32_t m_width;
		uint32_t m_height;
		RenderTargetType m_type;

		RenderPassBuilder m_renderPassBuilder;
	};
}

#endif // VK_GENERIC_RENDER_TARGET_H_
