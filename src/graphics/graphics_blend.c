
internal G_BlendSt
G_BlendStInit(void)
{
	G_BlendSt st = {0};
	
	st.enabled = true;
	
	st.constants[0] = 0.f;
	st.constants[1] = 0.f;
	st.constants[2] = 0.f;
	st.constants[3] = 0.f;
	
	st.write_mask[0] = true;
	st.write_mask[1] = true;
	st.write_mask[2] = true;
	st.write_mask[3] = true;

	st.colour.op  = VK_BLEND_OP_ADD;
	st.colour.src = VK_BLEND_FACTOR_ONE;
	st.colour.dst = VK_BLEND_FACTOR_ZERO;

	st.alpha.op  = VK_BLEND_OP_ADD;
	st.alpha.src = VK_BLEND_FACTOR_ONE;
	st.alpha.dst = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;

	st.logic_op_enabled = false;
	st.logic_op = VK_LOGIC_OP_COPY;

	return st;
}

internal G_DepthStencilSt
G_DepthStencilStInit(void)
{
	G_DepthStencilSt st = {0};

	st.depth_test_enabled = false;
	st.depth_write_enabled = false;
	st.depth_compare_op = VK_COMPARE_OP_LESS;

	st.depth_bounds_test_enabled = false;
	st.depth_bounds_min = 0.f;
	st.depth_bounds_max = 0.f;

	st.stencil_test_enabled = false;

	return st;
}
