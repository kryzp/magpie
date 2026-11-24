#ifndef RENDER_CLEAR_H
#define RENDER_CLEAR_H

namespace rnd {
struct Clear {
	float r = 0.f;
	float g = 0.f;
	float b = 0.f;
	float a = 0.f;

	float depth = 0.f;
	unsigned stencil = 0;

	static Clear Colour(float r, float g, float b, float a);
	static Clear DepthStencil(float depth, unsigned stencil);
};
}

#endif // RENDER_CLEAR_H
