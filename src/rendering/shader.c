#include "shader.h"

bool gfx_shader_program_is_compute(struct gfx_shader_program *program)
{
	return program->stages[0].stage == VK_SHADER_STAGE_COMPUTE_BIT;
}
