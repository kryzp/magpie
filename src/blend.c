
internal BlendState BlendStateDefault()
{
	BlendState state = {0};

	state.enabled = true;

	state.constants[0] = 0.f;
	state.constants[1] = 0.f;
	state.constants[2] = 0.f;
	state.constants[3] = 0.f;

	state.write_mask[0] = true;
	state.write_mask[1] = true;
	state.write_mask[2] = true;
	state.write_mask[3] = true;

	state.colour.op = VK_BLEND_OP_ADD;
	state.colour.src = VK_BLEND_FACTOR_ONE;
	state.colour.dst = VK_BLEND_FACTOR_ZERO;

	state.alpha.op = VK_BLEND_OP_ADD;
	state.alpha.src = VK_BLEND_FACTOR_ONE;
	state.alpha.dst = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;

	state.logic_op_enabled = false;
	state.logic_op = VK_LOGIC_OP_COPY;

	return state;
}

internal DepthStencilState DepthStencilStateDefault()
{
	DepthStencilState state = {0};

	state.depth_test_enabled = true;
	state.depth_write_enabled = true;
	state.depth_compare_op = VK_COMPARE_OP_LESS;
	state.depth_bounds_test_enabled = false;
	state.stencil_test_enabled = false;

	return state;
}
