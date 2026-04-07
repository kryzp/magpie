
internal R_Clear
R_ClearColour(f32 r, f32 g, f32 b, f32 a)
{
	R_Clear c = {0};
	c.r = r;
	c.g = g;
	c.b = b;
	c.a = a;

	return c;
}

internal R_Clear
R_ClearDepthStencil(f32 depth, u8 stencil)
{
	R_Clear c = {0};
	c.depth = depth;
	c.stencil = stencil;

	return c;
}
