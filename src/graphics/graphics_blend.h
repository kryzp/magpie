#ifndef GRAPHICS_BLEND_H
#define GRAPHICS_BLEND_H

typedef struct GFX_Blend GFX_Blend;
struct GFX_Blend
{
	VkBlendOp op;
	VkBlendFactor src;
	VkBlendFactor dst;
};

typedef struct GFX_BlendSt GFX_BlendSt;
struct GFX_BlendSt
{
	b32 enabled;

	f32 constants[4];
	b32 write_mask[4];

	GFX_Blend colour;
	GFX_Blend alpha;

	b32 logic_op_enabled;
	VkLogicOp logic_op;
};

typedef struct GFX_StencilSt GFX_StencilSt;
struct GFX_StencilSt
{
	VkStencilOp fail_op;
	VkStencilOp pass_op;
	VkStencilOp depth_fail_op;
	VkCompareOp compare_op;
	u32 write_mask;
	u32 reference;
};

typedef struct GFX_DepthStencilSt GFX_DepthStencilSt;
struct GFX_DepthStencilSt
{
	b32 depth_test_enabled;
	b32 depth_write_enabled;

	VkCompareOp depth_compare_op;

	b32 depth_bounds_test_enabled;
	f32 depth_bounds_min;
	f32 depth_bounds_max;
	
	b32 stencil_test_enabled;
	GFX_StencilSt stencil_front;
	GFX_StencilSt stencil_back;
};

internal GFX_BlendSt GFX_BlendStInit(void);
internal GFX_DepthStencilSt GFX_DepthStencilStInit(void);

#endif // GRAPHICS_BLEND_H
