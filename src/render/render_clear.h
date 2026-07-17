#ifndef RENDER_CLEAR_H
#define RENDER_CLEAR_H

typedef union R_Clear R_Clear;
union R_Clear
{
	struct
	{
		f32 r;
		f32 g;
		f32 b;
		f32 a;
	};

	struct
	{
		f32 depth;
		u8 stencil;
	};
};

static R_Clear R_ClearColour(f32 r, f32 g, f32 b, f32 a);
static R_Clear R_ClearDepthStencil(f32 depth, u8 stencil);

#endif // RENDER_CLEAR_H
