#ifndef GFX_BLEND_H
#define GFX_BLEND_H

#include <volk/volk.h>

#include "core/core_types.h"

struct gfx_blend {
	VkBlendOp op;
	VkBlendFactor src;
	VkBlendFactor dst;
};

struct gfx_blend_st {
	bool enabled;

	// [r, g, b, a]
	float constants[4];
	bool write_mask[4];

	struct gfx_blend colour;
	struct gfx_blend alpha;

	bool logic_op_enabled;
	VkLogicOp logic_op;
};

struct gfx_stencil_st {
	VkStencilOp fail_op;
	VkStencilOp pass_op;
	VkStencilOp depth_fail_op;
	VkCompareOp compare_op;
	u32 write_mask;
	u32 reference;
};

struct gfx_depth_stencil_st {
	bool depth_test_enabled;
	bool depth_write_enabled;
	VkCompareOp depth_compare_op;
	bool depth_bounds_test_enabled;
	float depth_bounds_min;
	float depth_bounds_max;
	bool stencil_test_enabled;
	struct gfx_stencil_st stencil_front;
	struct gfx_stencil_st stencil_back;
};

struct gfx_blend_st gfx_blend_st_default();
struct gfx_depth_stencil_st gfx_depth_stencil_st_default();

#endif // GFX_BLEND_H
