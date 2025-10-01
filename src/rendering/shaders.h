#ifndef GFX_SHADERS_H
#define GFX_SHADERS_H

#include "shader.h"
#include "device.h"

struct gfx_shaders {
#define GRAPHICS_SHADER_DEF(field, path_vert, path_comp) struct gfx_shader_program field;
#define COMPUTE_SHADER_DEF(field, path_comp) struct gfx_shader_program field;
#include "shaders.inc"
#undef GRAPHICS_SHADER_DEF
#undef COMPUTE_SHADER_DEF
};

void gfx_shaders_init(struct gfx_shaders *shaders, struct gfx_device *device);
void gfx_shaders_destroy(struct gfx_shaders *shaders, struct gfx_device *device);

#endif // GFX_SHADERS_H
