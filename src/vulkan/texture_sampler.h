#ifndef VK_TEXTURE_SAMPLER_H_
#define VK_TEXTURE_SAMPLER_H_

#include "third_party/volk.h"

namespace llt
{
	class TextureSampler
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

		TextureSampler();
		TextureSampler(const Style &style);
		~TextureSampler();

		void cleanUp();

		/*
		 * Create a sampler with some properties and amount of mip levels
		 */
		VkSampler bind();

		VkSampler sampler() const;

		Style style;

		bool operator == (const TextureSampler &other) const { return this->style == other.style; }
		bool operator != (const TextureSampler &other) const { return this->style != other.style; }

	private:
		VkSampler m_sampler;
	};
}

#endif // VK_TEXTURE_SAMPLER_H_
