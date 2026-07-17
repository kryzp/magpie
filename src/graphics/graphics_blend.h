#ifndef GRAPHICS_BLEND_H
#define GRAPHICS_BLEND_H

typedef struct G_Blend G_Blend;
struct G_Blend
{
	VkBlendOp op;
	VkBlendFactor src;
	VkBlendFactor dst;
};

typedef struct G_BlendSt G_BlendSt;
struct G_BlendSt
{
	b32 enabled;

	f32 constants[4];
	b32 write_mask[4];

	G_Blend colour;
	G_Blend alpha;

	b32 logic_op_enabled;
	VkLogicOp logic_op;
};

typedef struct G_StencilSt G_StencilSt;
struct G_StencilSt
{
	VkStencilOp fail_op;
	VkStencilOp pass_op;
	VkStencilOp depth_fail_op;
	VkCompareOp compare_op;
	u32 write_mask;
	u32 reference;
};

typedef struct G_DepthStencilSt G_DepthStencilSt;
struct G_DepthStencilSt
{
	b32 depth_test_enabled;
	b32 depth_write_enabled;

	VkCompareOp depth_compare_op;

	b32 depth_bounds_test_enabled;
	f32 depth_bounds_min;
	f32 depth_bounds_max;
	
	b32 stencil_test_enabled;
	G_StencilSt stencil_front;
	G_StencilSt stencil_back;
};

static G_BlendSt G_BlendStInit(void);
static G_DepthStencilSt G_DepthStencilStInit(void);

#endif // GRAPHICS_BLEND_H
