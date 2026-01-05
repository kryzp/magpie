#ifndef GFX_RENDER_GRAPH_H
#define GFX_RENDER_GRAPH_H

#include "core/core_types.h"
#include "core/core_string.h"

#include "device.h"
#include "resources.h"
#include "render_scene.h"

#define GFX_RENDER_STAGE_MAX_ATTACHMENTS 8
#define GFX_RENDER_STAGE_MAX_FEATURE_RENDERERS 16
#define GFX_RENDER_STAGE_MAX_TEXTURE_VIEWS 16
#define GFX_RENDER_STAGE_MAX_BUFFERS 16

struct gfx_render_context {
	struct gfx_device *device;
	struct gfx_command_buffer *cmd;
	const struct gfx_render_view *view;
	struct hash_table *attachments;
};

enum gfx_render_size_class {
	GFX_RENDER_SIZE_swapchain_relative,
	GFX_RENDER_SIZE_absolute,
	GFX_RENDER_SIZE_max_enum
};

struct gfx_render_attachment {
	bool is_depth_stencil;
	VkFormat colour_format;
	enum gfx_render_size_class size_class;
	float size_x;
	float size_y;
};

void gfx_render_attachment_init_colour(struct gfx_render_attachment *attachment,
				       enum gfx_render_size_class size_class,
				       VkFormat format, float size_x, float size_y);

void gfx_render_attachment_init_depth_stencil(struct gfx_render_attachment *attachment,
					      enum gfx_render_size_class size_class,
					      float size_x, float size_y);

typedef void (*gfx_render_feature_record_t)(void *state, const struct gfx_render_context *ctx);

enum gfx_render_stage_type {
	GFX_RENDER_STAGE_graphics,
	GFX_RENDER_STAGE_compute,
	GFX_RENDER_STAGE_post,
	GFX_RENDER_STAGE_transfer,
	GFX_RENDER_STAGE_mipmap,
	GFX_RENDER_STAGE_present,
	GFX_RENDER_STAGE_max_enum
};

struct gfx_render_clear {
	bool enabled;

	union {
		v4 colour;
		
		struct {
			float depth;
			u8 stencil;
		};
	}
};

struct gfx_render_stage {
	enum gfx_render_stage_type type;

	struct gfx_render_view render_view;
	
	u32 multi_view_mask;

	//bool resize_output;

	int feature_renderer_count;
	struct {
		void *state;
		gfx_render_feature_record_t record;
	} feature_renderers[GFX_RENDER_STAGE_MAX_FEATURE_RENDERERS];

	u32 attachment_count;
	struct {
		struct string8 name;
		struct gfx_render_clear clear;
	} attachments[GFX_RENDER_STAGE_MAX_ATTACHMENTS];

	u32 texture_view_count;
	struct {
		struct gfx_texture_view *view;
		enum gfx_texture_access_type access_type;
	} texture_views[GFX_RENDER_STAGE_MAX_TEXTURE_VIEWS];
	
	u32 buffer_count;
	struct {
		struct gfx_buffer *buffer;
		enum gfx_buffer_access_type access_type;
	} buffers[GFX_RENDER_STAGE_MAX_BUFFERS];
	
	struct gfx_texture *mipmap_texture;
};

void gfx_render_stage_init(struct gfx_render_stage *stage, enum gfx_render_stage_type type);

struct gfx_render_graph;

void gfx_render_stage_execute(struct gfx_render_stage *stage, struct gfx_device *device, struct gfx_command_buffer *cmd, struct gfx_swapchain *swapchain, struct gfx_render_graph *graph);

void gfx_render_stage_set_render_view(struct gfx_render_stage *stage,
				      struct gfx_render_view view);

void gfx_render_stage_set_multi_view_mask(struct gfx_render_stage *stage,
					  u32 view_mask);

void gfx_render_stage_add_feature_renderer(struct gfx_render_stage *stage,
					   void *state, gfx_render_feature_record_t record);

void gfx_render_stage_add_attachment(struct gfx_render_stage *stage,
				     struct string8 name,
				     struct gfx_render_clear *clear);

void gfx_render_stage_add_texture_view(struct gfx_render_stage *stage,
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
	
	u32 attachment_count;
	struct {
		struct string8 name;
		struct gfx_render_attachment attachment;
		struct gfx_texture *texture;
	} attachments[32];
};

void gfx_render_graph_init(struct gfx_render_graph *graph);
void gfx_render_graph_destroy(struct gfx_render_graph *graph, struct gfx_device *device);

void gfx_render_graph_attach(struct gfx_render_graph *graph,
			     struct string8 name,
			     struct gfx_render_attachment *attachment);

void gfx_render_graph_reset(struct gfx_render_graph *graph);

void gfx_render_graph_update(struct gfx_render_graph *graph);

void gfx_render_graph_render(struct gfx_render_graph *graph,
			     struct gfx_device *device,
			     struct gfx_swapchain *swapchain,
			     struct gfx_command_buffer *cmd);

void gfx_render_graph_resize(struct gfx_render_graph *graph, struct gfx_device *device, u32 width, u32 height);

void gfx_render_graph_push(struct gfx_render_graph *graph, struct gfx_render_stage *stage);

#endif // GFX_RENDER_GRAPH_H
