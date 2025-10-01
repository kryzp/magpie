#ifndef STAGE_H
#define STAGE_H

#include "../render_graph.h"

struct stage_clear_draw_indirect_input {
	struct gfx_buffer *clear_indirect_buffer;
	struct gfx_buffer *draw_indirect_buffer;
};

void stage_add_clear_draw_indirect(struct gfx_render_graph *graph,
				   struct stage_clear_draw_indirect_input *input);

/*
struct stage_render_scene_input {
	struct gfx_environment_probe *probe;
	struct gfx_gbuffer *gbuffer;
	struct gfx_texture_view *lighting;
	struct gfx_buffer *frame_data_buffer;
	struct gfx_buffer *object_buffer;
	struct gfx_buffer *instance_buffer;
	struct gfx_buffer *indirect_buffer;
	struct gfx_buffer *light_buffer;
};
*/

struct stage_skybox_input {
	struct gfx_texture_view *skybox;
	struct gfx_texture_view *target;
	struct gfx_texture_view *depth;
	struct gfx_buffer *frame_data_buffer;
};

void stage_add_skybox(struct gfx_render_graph *graph,
		      struct stage_skybox_input *input);

struct stage_post_processing_input {
	struct gfx_texture_view *input;
	struct gfx_texture_view *output;
	float exposure;
};

void stage_add_post_processing(struct gfx_render_graph *graph,
			       struct stage_post_processing_input *input);

struct stage_blit_lighting_to_swapchain_input {
	struct gfx_texture_view *lighting;
	struct gfx_texture_view *swapchain;
};

void stage_add_blit_lighting_to_swapchain(struct gfx_render_graph *graph,
					  struct stage_blit_lighting_to_swapchain_input *input);

struct stage_hdr_texture_to_cubemap_input {
	struct gfx_texture_view *texture;
	struct gfx_texture_view *output;
};

void stage_add_hdr_texture_to_cubemap(struct gfx_render_graph *graph,
				      struct stage_hdr_texture_to_cubemap_input *input);

#endif // STAGE_H
