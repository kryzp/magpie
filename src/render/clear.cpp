#include "render/clear.h"

rnd::Clear rnd::Clear::Colour(float r, float g, float b, float a)
{
	Clear c;
	c.r = r;
	c.g = g;
	c.b = b;
	c.a = a;
	return c;
}

rnd::Clear rnd::Clear::DepthStencil(float depth, unsigned stencil)
{
	Clear c;
	c.depth = depth;
	c.stencil = stencil;
	return c;
}
