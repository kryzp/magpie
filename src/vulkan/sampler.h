#pragma once

#include "third_party/volk.h"

#include "bindless.h"

namespace mgp
{
	class VulkanCore;

	class Sampler
	{
	public:
		struct Style
		{
			VkFilter filter;
			VkSamplerAddressMode wrapX;
			VkSamplerAddressMode wrapY;
			VkSamplerAddressMode wrapZ;
			VkBorderColor borderColour;

			Style()
				: filter(VK_FILTER_LINEAR)
				, wrapX(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE)
				, wrapY(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE)
				, wrapZ(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE)
				, borderColour(VK_BORDER_COLOR_INT_OPAQUE_BLACK)
			{
			}

			Style(
				VkFilter filter,
				VkSamplerAddressMode wrapX = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
				VkSamplerAddressMode wrapY = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
				VkSamplerAddressMode wrapZ = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
				VkBorderColor borderColour = VK_BORDER_COLOR_INT_OPAQUE_BLACK
			)
				: filter(filter)
				, wrapX(wrapX)
				, wrapY(wrapY)
				, wrapZ(wrapZ)
				, borderColour(borderColour)
			{
			}

			bool operator == (const Style &other) const { return this->filter == other.filter && this->wrapX == other.wrapX && this->wrapY == other.wrapY && this->wrapZ == other.wrapZ; }
			bool operator != (const Style &other) const { return !(*this == other); }
		};

		Sampler(VulkanCore *core, const Style &style);
		~Sampler();

		VkSampler getHandle() const;
		const Style &getStyle() const;

		bindless::Handle getBindlessHandle() const;

		bool operator == (const Sampler &other) const { return this->m_style == other.m_style; }
		bool operator != (const Sampler &other) const { return this->m_style != other.m_style; }

	private:
		VkSampler m_sampler;
		Style m_style;

		bindless::Handle m_bindlessHandle;

		VulkanCore *m_core;
	};
}
