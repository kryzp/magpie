#ifndef GFX_RENDER_GRAPH_H
#define GFX_RENDER_GRAPH_H

#include "core/core_types.h"

#include "device.h"
#include "resources.h"

struct gfx_render_state {
	struct gfx_device *device;
	struct gfx_command_buffer *cmd;
	const struct gfx_render_view *view;
};

enum gfx_render_size_class {
	GFX_RENDER_SIZE_swapchain_relative,
	GFX_RENDER_SIZE_absolute,
	GFX_RENDER_SIZE_max_enum
};

struct gfx_render_stage_attachment {
	struct gfx_texture_view *view;
	enum gfx_render_size_class size_class;
	float size_x;
	float size_y;
	bool clear_colour;
	bool clear_depth_stencil;
	v4 clear_colour_value;
	float clear_depth_value;
	u8 clear_stencil_value;
};

void gfx_render_stage_attachment_init_colour(struct gfx_render_stage_attachment *attachment,
					     struct gfx_texture_view *view,
					     enum gfx_render_size_class size_class,
					     bool clear, v4 clear_colour);

void gfx_render_stage_attachment_init_depth_stencil(struct gfx_render_stage_attachment *attachment,
						    struct gfx_texture_view *view,
						    enum gfx_render_size_class size_class,
						    bool clear, float clear_depth, u8 clear_stencil);

#define GFX_RENDER_FEATURE_MAX_CONTEXT_BYTES 64

typedef void (*gfx_render_feature_record_t)(void *self, struct gfx_render_state *rs);

enum gfx_render_stage_type {
	GFX_RENDER_STAGE_graphics,
	GFX_RENDER_STAGE_compute,
	GFX_RENDER_STAGE_post,
	GFX_RENDER_STAGE_transfer,
	GFX_RENDER_STAGE_mipmap,
	GFX_RENDER_STAGE_present,
	GFX_RENDER_STAGE_max_enum
};

struct gfx_render_stage {
	enum gfx_render_stage_type type;

	struct gfx_render_view *render_view;
	
	u32 graphics_view_mask;

	//bool resize_output;

	int feature_count;
	struct {
		u8 self[GFX_RENDER_FEATURE_MAX_CONTEXT_BYTES];
		gfx_render_feature_record_t record;
	} features[16];

	u32 attachment_count;
	struct gfx_render_stage_attachment attachments[8];

	u32 view_count;
	struct {
		struct gfx_texture_view *view;
		enum gfx_texture_access_type access_type;
	} views[16];
	
	u32 buffer_count;
	struct {
		struct gfx_buffer *buffer;
		enum gfx_buffer_access_type access_type;
	} buffers[16];
	
	struct gfx_texture *mipmap_texture;
};

void gfx_render_stage_init(struct gfx_render_stage *stage, enum gfx_render_stage_type type);

void gfx_render_stage_execute(struct gfx_render_stage *stage, struct gfx_device *device, struct gfx_command_buffer *cmd, struct gfx_swapchain *swapchain);

void gfx_render_stage_load_resources(struct gfx_render_stage *stage, struct gfx_device *device, struct gfx_resource_database *db);
void gfx_render_stage_resize(struct gfx_render_stage *stage, struct gfx_device *device, u32 width, u32 height);

void gfx_render_stage_set_render_view(struct gfx_render_stage *stage,
				      struct gfx_render_view *view);

void gfx_render_stage_set_graphics_view_mask(struct gfx_render_stage *stage,
					     u32 view_mask);

void gfx_render_stage_add_feature(struct gfx_render_stage *stage,
				  u64 self_size, void *self,
				  gfx_render_feature_record_t record);

void gfx_render_stage_add_attachment(struct gfx_render_stage *stage,
				     const struct gfx_render_stage_attachment *attachment);

void gfx_render_stage_add_view(struct gfx_render_stage *stage,
			       struct gfx_texture_view *view,
			       enum gfx_texture_access_type access_type);

void gfx_render_stage_add_buffer(struct gfx_render_stage *stage,
				 struct gfx_buffer *buffer,
				 enum gfx_buffer_access_type access_type);

struct gfx_render_graph {
	struct gfx_resource_database resource_database;
	struct gfx_resource_lookup resource_lookup;
	u32 stage_count;
	struct gfx_render_stage stages[32];
};

void gfx_render_graph_init(struct gfx_render_graph *graph);
void gfx_render_graph_destroy(struct gfx_render_graph *graph);

void gfx_render_graph_reset(struct gfx_render_graph *graph);

void gfx_render_graph_update(struct gfx_render_graph *graph);

void gfx_render_graph_render(struct gfx_render_graph *graph,
			     struct gfx_device *device,
			     struct gfx_swapchain *swapchain,
			     struct gfx_command_buffer *cmd);

void gfx_render_graph_load_resources(struct gfx_render_graph *graph, struct gfx_device *device);
void gfx_render_graph_resize(struct gfx_render_graph *graph, struct gfx_device *device, u32 width, u32 height);

void gfx_render_graph_push(struct gfx_render_graph *graph, struct gfx_render_stage *stage);

#endif // GFX_RENDER_GRAPH_H
