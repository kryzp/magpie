#ifndef GFX_SHADER_H
#define GFX_SHADER_H

#include <volk/volk.h>

#include "core/core_types.h"

struct gfx_shader_stage {
	VkShaderStageFlagBits stage;
	VkShaderModule module;
	u32 push_constant_size;
};

/*
 * Shaders are assumed to be fully bindless.
 */
struct gfx_shader_program {
	u32 push_constant_size;
	u32 stage_count;
	struct gfx_shader_stage stages[2];
};

bool gfx_shader_program_is_compute(struct gfx_shader_program *program);

#endif // GFX_SHADER_H
