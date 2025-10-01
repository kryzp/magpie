#include "pipeline.h"
#include "blend.h"

struct gfx_graphics_pipeline_def gfx_graphics_pipeline_def_init(struct gfx_shader_program *program)
{
	struct gfx_graphics_pipeline_def def = {0};
	def.program = program;
	def.cull_mode = VK_CULL_MODE_BACK_BIT;
	def.front_face = VK_FRONT_FACE_CLOCKWISE;
	def.blend_state = gfx_blend_st_default();
	def.depth_stencil_state = gfx_depth_stencil_st_default();
	def.has_depth_attachment = false;
	def.samples = VK_SAMPLE_COUNT_1_BIT;
	def.min_sample_shading_enabled = true;
	def.min_sample_shading = 0.2f;
	def.view_mask = 0;

	return def;
}

struct gfx_compute_pipeline_def gfx_compute_pipeline_def_init(struct gfx_shader_program *program)
{
	struct gfx_compute_pipeline_def def = {0};
	def.program = program;
	
	return def;
}
