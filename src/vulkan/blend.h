#ifndef BLEND_H_
#define BLEND_H_

#include "third_party/volk.h"

namespace llt
{
	struct Blend
	{
		VkBlendOp op;
		VkBlendFactor src;
		VkBlendFactor dst;

		Blend()
			: op(VK_BLEND_OP_ADD)
			, src(VK_BLEND_FACTOR_ONE)
			, dst(VK_BLEND_FACTOR_ZERO)
		{
		}

		Blend(VkBlendOp op, VkBlendFactor src, VkBlendFactor dst)
			: op(op)
			, src(src)
			, dst(dst)
		{
		}
	};

	struct BlendState
	{
		float blendConstants[4]; // [r, g, b, a]
		bool writeMask[4]; // [r, g, b, a]

		Blend colour;
		Blend alpha;

		bool blendOpEnabled;
		VkLogicOp blendOp;

		bool enabled;

		BlendState()
			: blendConstants{0.0f, 0.0f, 0.0f, 0.0f}
			, writeMask{true, true, true, true}
			, colour()
			, alpha()
			, blendOpEnabled(false)
			, blendOp(VK_LOGIC_OP_COPY)
			, enabled(true)
		{
		}
	};
}

#endif // BLEND_H_
