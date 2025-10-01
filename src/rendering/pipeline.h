#ifndef GFX_PIPELINE_H
#define GFX_PIPELINE_H

#include <volk/volk.h>

#include "core/core_math.h"

#include "shader.h"
#include "blend.h"

#define GFX_MAX_COLOUR_ATTACHMENTS 32

struct gfx_render_info {
	u32 width;
	u32 height;
	VkSampleCountFlagBits samples;
	u32 view_mask;
	u32 colour_attachment_count;
	VkRenderingAttachmentInfo colour_attachments[GFX_MAX_COLOUR_ATTACHMENTS];
	VkRenderingAttachmentInfo depth_attachment;
};

struct gfx_graphics_pipeline_def {
	struct gfx_shader_program *program;
	
	VkCullModeFlags cull_mode;
	VkFrontFace front_face;

	struct gfx_blend_st blend_state;
	struct gfx_depth_stencil_st depth_stencil_state;

	int colour_attachment_count;
	VkFormat colour_attachment_formats[GFX_MAX_COLOUR_ATTACHMENTS];

	bool has_depth_attachment;

	VkSampleCountFlagBits samples;

	bool min_sample_shading_enabled;
	float min_sample_shading;

	u32 view_mask;
};

struct gfx_compute_pipeline_def {
	struct gfx_shader_program *program;
};

struct gfx_graphics_pipeline_def gfx_graphics_pipeline_def_init(struct gfx_shader_program *program);
struct gfx_compute_pipeline_def gfx_compute_pipeline_def_init(struct gfx_shader_program *program);

struct gfx_pipeline_st {
	VkPipeline pipeline;
	VkPipelineLayout layout;
	VkPipelineBindPoint bind_point;
};

#endif // GFX_PIPELINE_H
