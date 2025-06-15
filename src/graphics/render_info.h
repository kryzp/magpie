#pragma once

#include <vector>

#include <Volk/volk.h>

#include "core/common.h"

#include "math/colour.h"

#include "image.h"
#include "image_view.h"

namespace mgp
{
	class ImageView;

	class RenderInfo
	{
	public:
		RenderInfo()
			: m_width(0)
			, m_height(0)
			, m_samples(VK_SAMPLE_COUNT_1_BIT)
			, m_colourFormats()
			, m_colourAttachments()
			, m_depthAttachment()
		{
		}

		void addColourAttachment(VkAttachmentLoadOp loadOp, ImageView *view, ImageView *resolve, const Colour &clear = Colour::black())
		{
			glm::vec4 clearColour = clear.getPremultiplied().getDisplayColour();

			VkRenderingAttachmentInfo attachment = {};
			attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
			attachment.imageView = view->getHandle();
			attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			attachment.loadOp = loadOp;
			attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			attachment.clearValue = { .color = { clearColour.x, clearColour.y, clearColour.z, clearColour.a } };

			if (resolve)
			{
				attachment.resolveImageView = resolve->getHandle();
				attachment.resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				attachment.resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT;
			}
			else
			{
				attachment.resolveImageView = VK_NULL_HANDLE;
				attachment.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				attachment.resolveMode = VK_RESOLVE_MODE_NONE;
			}

			m_colourAttachments.push_back(attachment);
			m_colourFormats.push_back(view->getImage()->getFormat());
		}

		void addDepthAttachment(VkAttachmentLoadOp loadOp, ImageView *view, ImageView *resolve, float depthClear = 1.0f, uint32_t stencilClear = 0)
		{
			m_depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
			m_depthAttachment.imageView = view->getHandle();
			m_depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			m_depthAttachment.loadOp = loadOp;
			m_depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			m_depthAttachment.clearValue = { .depthStencil = { depthClear, stencilClear } };

			if (resolve)
			{
				m_depthAttachment.resolveImageView = resolve->getHandle();
				m_depthAttachment.resolveImageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
				m_depthAttachment.resolveMode = VK_RESOLVE_MODE_SAMPLE_ZERO_BIT;
			}
			else
			{
				m_depthAttachment.resolveImageView = VK_NULL_HANDLE;
				m_depthAttachment.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				m_depthAttachment.resolveMode = VK_RESOLVE_MODE_NONE;
			}
		}

		const std::vector<VkRenderingAttachmentInfo> &getColourAttachments() const { return m_colourAttachments; }
		const VkRenderingAttachmentInfo &getDepthAttachment() const { return m_depthAttachment; }

		void setClearColour(int idx, const Colour &colour)
		{
			glm::vec4 clearColour = colour.getPremultiplied().getDisplayColour();

			m_colourAttachments[idx].clearValue.color.float32[0] = clearColour.x;
			m_colourAttachments[idx].clearValue.color.float32[1] = clearColour.y;
			m_colourAttachments[idx].clearValue.color.float32[2] = clearColour.z;
			m_colourAttachments[idx].clearValue.color.float32[3] = clearColour.w;
		}

		void setClearDepth(float depth, uint32_t stencil)
		{
			m_depthAttachment.clearValue.depthStencil.depth = depth;
			m_depthAttachment.clearValue.depthStencil.stencil = stencil;
		}

		void setMSAA(VkSampleCountFlagBits samples) { m_samples = samples; }
		VkSampleCountFlagBits getMSAA() const { return m_samples; }

		const std::vector<VkFormat> &getColourAttachmentFormats() const
		{
			return m_colourFormats;
		}

		void setSize(uint32_t width, uint32_t height)
		{
			m_width = width;
			m_height = height;
		}

		uint32_t getWidth() const { return m_width; }
		uint32_t getHeight() const { return m_height; }

		VkRenderingInfo getVkRenderingInfo() const
		{
			VkRenderingInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
			info.renderArea.offset = { 0, 0 };
			info.renderArea.extent = { m_width, m_height };
			info.layerCount = 1;
			info.colorAttachmentCount = m_colourAttachments.size();
			info.pColorAttachments = m_colourAttachments.data();
			info.pDepthAttachment = m_depthAttachment.imageView ? &m_depthAttachment : nullptr;
			info.pStencilAttachment = nullptr;

			return info;
		}

		uint64_t getHash() const
		{
			uint64_t h = 0;

			hash::combine(&h, &m_width);
			hash::combine(&h, &m_height);
			hash::combine(&h, &m_samples);
			hash::combine(&h, &m_depthAttachment);

			for (auto &c : m_colourAttachments)
				hash::combine(&h, &c);

			return h;
		}

	private:
		uint32_t m_width;
		uint32_t m_height;

		VkSampleCountFlagBits m_samples;

		std::vector<VkFormat> m_colourFormats;

		std::vector<VkRenderingAttachmentInfo> m_colourAttachments;
		VkRenderingAttachmentInfo m_depthAttachment;
	};
}
