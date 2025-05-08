#pragma once

#include <Volk/volk.h>

namespace mgp
{
	struct Blend
	{
		VkBlendOp op;
		VkBlendFactor src;
		VkBlendFactor dst;

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
			, colour(VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO)
			, alpha(VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA)
			, blendOpEnabled(false)
			, blendOp(VK_LOGIC_OP_COPY)
			, enabled(true)
		{
		}
	};
}
