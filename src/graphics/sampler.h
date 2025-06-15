#pragma once

#include <Volk/volk.h>

namespace mgp
{	
	struct SamplerStyle
	{
		VkFilter filter;
		VkSamplerAddressMode wrapX;
		VkSamplerAddressMode wrapY;
		VkSamplerAddressMode wrapZ;
		VkBorderColor borderColour;

		SamplerStyle()
			: filter(VK_FILTER_LINEAR)
			, wrapX(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE)
			, wrapY(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE)
			, wrapZ(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE)
			, borderColour(VK_BORDER_COLOR_INT_OPAQUE_BLACK)
		{
		}

		SamplerStyle(
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

		bool operator == (const SamplerStyle &other) const { return this->filter == other.filter && this->wrapX == other.wrapX && this->wrapY == other.wrapY && this->wrapZ == other.wrapZ; }
		bool operator != (const SamplerStyle &other) const { return !(*this == other); }
	};
	
	class GraphicsCore;

	class Sampler
	{
	public:
		Sampler(GraphicsCore *gfx, const SamplerStyle &style);
		~Sampler();

		const SamplerStyle &getStyle() const { return m_style; }

		VkSampler getHandle() const { return m_sampler; }

	private:
		GraphicsCore *m_gfx;

		VkSampler m_sampler;
		SamplerStyle m_style;
	};
}
