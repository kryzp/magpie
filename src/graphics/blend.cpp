#include "graphics/blend.h"

#include <cstring>

gfx::BlendState::BlendState()
{
	memset(this, 0, sizeof(*this));

	enabled = true;

	constants[0] = 0.f;
	constants[1] = 0.f;
	constants[2] = 0.f;
	constants[3] = 0.f;

	write_mask[0] = true;
	write_mask[1] = true;
	write_mask[2] = true;
	write_mask[3] = true;

	colour.op = VK_BLEND_OP_ADD;
	colour.src = VK_BLEND_FACTOR_ONE;
	colour.dst = VK_BLEND_FACTOR_ZERO;

	alpha.op = VK_BLEND_OP_ADD;
	alpha.src = VK_BLEND_FACTOR_ONE;
	alpha.dst = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;

	logic_op_enabled = false;
	logic_op = VK_LOGIC_OP_COPY;
}

gfx::BlendState gfx::BlendState::Disabled()
{
	BlendState state;
	state.enabled = false;

	return state;
}

gfx::BlendState gfx::BlendState::Uniform(VkBlendOp op, VkBlendFactor src, VkBlendFactor dst)
{
	BlendState state;
	state.enabled = true;
	state.colour.op = op;
	state.colour.src = src;
	state.colour.dst = dst;
	state.alpha.op = op;
	state.alpha.src = src;
	state.alpha.dst = dst;

	return state;
}

gfx::DepthStencilState::DepthStencilState()
{
	memset(this, 0, sizeof(*this));

	depth_test_enabled = false;
	depth_write_enabled = false;
	depth_compare_op = VK_COMPARE_OP_LESS;

	depth_bounds_test_enabled = false;
	depth_bounds_min = 0.f;
	depth_bounds_max = 0.f;

	stencil_test_enabled = false;
}
