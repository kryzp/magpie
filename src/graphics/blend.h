#pragma once

#include <volk/volk.h>

#include "core/types.h"

namespace gfx
{

struct Blend {
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

struct BlendState {
	bool enabled;

	// r, g, b, a
	float constants[4];
	bool write_mask[4];

	Blend colour;
	Blend alpha;

	bool logic_op_enabled;
	VkLogicOp logic_op;

	BlendState()
		: enabled(true)
		, constants{0.f, 0.f, 0.f, 0.f}
		, write_mask{true, true, true, true}
	    , colour(VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO)
        , alpha(VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA)
        , logic_op_enabled(false)
        , logic_op(VK_LOGIC_OP_COPY)
	{
	}
};

struct StencilState {
	VkStencilOp fail_op;
	VkStencilOp pass_op;
	VkStencilOp depth_fail_op;
	VkCompareOp compare_op;
	u32 write_mask;
	u32 reference;
};

struct DepthStencilState {
	bool depth_test_enabled = true;
	bool depth_write_enabled = true;
	VkCompareOp depth_compare_op = VK_COMPARE_OP_LESS;
	bool depth_bounds_test_enabled = false;
	float depth_bounds_min = 0.f;
	float depth_bounds_max = 0.f;
	bool stencil_test_enabled = true;
	StencilState stencil_front = {};
	StencilState stencil_back = {};
};

}
