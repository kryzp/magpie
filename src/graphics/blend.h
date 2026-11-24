#ifndef GRAPHICS_BLEND_H
#define GRAPHICS_BLEND_H

#include "core/types.h"

#include <volk/volk.h>

namespace gfx {

// hashed byte for byte
struct Blend {
	VkBlendOp op;
	VkBlendFactor src;
	VkBlendFactor dst;
};

// hashed byte for byte
struct BlendState {
	unsigned enabled;

	float constants[4];
	unsigned write_mask[4];

	Blend colour;
	Blend alpha;

	unsigned logic_op_enabled;
	VkLogicOp logic_op;

	BlendState();

	static BlendState Disabled();
	static BlendState Uniform(VkBlendOp op, VkBlendFactor src, VkBlendFactor dst);
};

// hashed byte for byte
struct StencilState {
	VkStencilOp fail_op;
	VkStencilOp pass_op;
	VkStencilOp depth_fail_op;
	VkCompareOp compare_op;
	unsigned write_mask;
	unsigned reference;
};

// hashed byte for byte
struct DepthStencilState {
	unsigned depth_test_enabled;
	unsigned depth_write_enabled;

	VkCompareOp depth_compare_op;

	unsigned depth_bounds_test_enabled;
	float depth_bounds_min;
	float depth_bounds_max;

	unsigned stencil_test_enabled;
	StencilState stencil_front;
	StencilState stencil_back;

	DepthStencilState();
};

}

#endif // GRAPHICS_BLEND_H
