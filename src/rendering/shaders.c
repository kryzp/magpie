#include "shaders.h"

#include "core/core_string.h"

/*
 * Shaders are defined by using X-Macros.
 * I honestly don't know why more people don't use them.
 */

void gfx_shaders_init(struct gfx_shaders *shaders, struct gfx_device *device)
{
#define GRAPHICS_SHADER_DEF(field, path_vert, path_frag)		\
	{								\
		struct string8 files[] = { str8(path_vert), str8(path_frag) }; \
		shaders->field = gfx_device_shader_program_create(device, 2, files); \
	}
	
#define COMPUTE_SHADER_DEF(field, path_comp)				\
	{								\
		struct string8 file = str8(path_comp);			\
		shaders->field = gfx_device_shader_program_create(device, 1, &file); \
	}
	
#include "shaders.inc"

#undef GRAPHICS_SHADER_DEF
#undef COMPUTE_SHADER_DEF

	debug_log("Loaded shaders.");
}

void gfx_shaders_destroy(struct gfx_shaders *shaders, struct gfx_device *device)
{
#define GRAPHICS_SHADER_DEF(field, path_vert, path_frag) gfx_device_shader_program_destroy(device, &shaders->field);
#define COMPUTE_SHADER_DEF(field, path_comp) gfx_device_shader_program_destroy(device, &shaders->field);
#include "shaders.inc"
#undef GRAPHICS_SHADER_DEF
#undef COMPUTE_SHADER_DEF
}
