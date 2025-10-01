#include "render_graph.h"

#include "core/core_scratch.h"

#include "sync.h"

static void gfx_render_stage_attachment_get_absolute_size(struct gfx_render_stage_attachment *attachment,
							  struct gfx_swapchain *swapchain,
							  u32 *width, u32 *height)
{
	switch (attachment->size_class) {
	case GFX_RENDER_SIZE_swapchain_relative:
		*width = swapchain->width * attachment->size_x;
		*height = swapchain->height * attachment->size_y;
		break;
	case GFX_RENDER_SIZE_absolute:
		*width = attachment->size_x;
		*height = attachment->size_y;
		break;
	}
}

void gfx_render_stage_attachment_init_swapchain(struct gfx_render_stage_attachment *attachment,
						bool clear, v4 clear_colour)
{
	attachment->view = NULL;
	attachment->size_class = GFX_RENDER_SIZE_swapchain_relative;
	attachment->clear_colour = clear;
	attachment->clear_colour_value = clear_colour;
	attachment->size_x = 1.f;
	attachment->size_y = 1.f;
}

void gfx_render_stage_attachment_init_colour(struct gfx_render_stage_attachment *attachment,
					     struct gfx_texture_view *view,
					     enum gfx_render_size_class size_class,
					     bool clear, v4 clear_colour)
{
	attachment->view = view;
	attachment->size_class = size_class;
	attachment->clear_colour = clear;
	attachment->clear_colour_value = clear_colour;

	switch (size_class) {
	case GFX_RENDER_SIZE_swapchain_relative:
		attachment->size_x = 1.f;
		attachment->size_y = 1.f;
		break;
	case GFX_RENDER_SIZE_absolute:
		attachment->size_x = view->parent->width >> view->base_level;
		attachment->size_y = view->parent->height >> view->base_level;
		break;
	}
}

void gfx_render_stage_attachment_init_depth_stencil(struct gfx_render_stage_attachment *attachment,
						    struct gfx_texture_view *view,
						    enum gfx_render_size_class size_class,
						    bool clear, float clear_depth, u8 clear_stencil)
{
	attachment->view = view;
	attachment->size_class = size_class;
	attachment->clear_depth_stencil = clear;
	attachment->clear_depth_value = clear_depth;
	attachment->clear_stencil_value = clear_stencil;

	switch (size_class) {
	case GFX_RENDER_SIZE_swapchain_relative:
		attachment->size_x = 1.f;
		attachment->size_y = 1.f;
		break;
	case GFX_RENDER_SIZE_absolute:
		attachment->size_x = view->parent->width >> view->base_level;
		attachment->size_y = view->parent->height >> view->base_level;
		break;
	}
}

static VkRenderingAttachmentInfo vk_rendering_attachment_from_stage_attachment(struct gfx_render_stage_attachment *attachment)
{
	VkRenderingAttachmentInfo info = {0};
	info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	info.imageView = attachment->view->handle;
	info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	info.loadOp = (attachment->clear_colour || attachment->clear_depth_stencil) ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
	info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	info.resolveImageView = VK_NULL_HANDLE;
	info.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	info.resolveMode = VK_RESOLVE_MODE_NONE;

	if (attachment->clear_colour) {
		v4 clear_colour = attachment->clear_colour_value;
		info.clearValue = (VkClearValue){ .color = { clear_colour.r, clear_colour.g, clear_colour.b, clear_colour.a } };
        } else {
		float clear_depth = attachment->clear_depth_value;
		u8 clear_stencil = attachment->clear_stencil_value;
		info.clearValue = (VkClearValue){ .depthStencil = { clear_depth, clear_stencil } };
	}
	
	return info;
}

// TODO: LAYOUT TRANSITIONS ONLY TRANSITION THE FIRST (0) ASPECT OF THE IMAGE.
//       THEREFORE IN THE CASE OF DEPTH_STENCIL, WE ONLY TRANSITION DEPTH.
//       SO STENCIL WILL NOT WORK.
//       FIX.

static void gfx_render_stage_transition_texture(struct gfx_texture *texture,
						enum gfx_texture_access_type dst_access,
						VkImageMemoryBarrier2 *barriers, u32 *barrier_count)
{
	struct gfx_texture_access dst_access_info = gfx_sync_get_dst_texture_access(dst_access);

	for (u32 i = 0; i < texture->mipmap_count; i++) {
		for (u32 j = 0; j < gfx_texture_layer_count(texture); j++) {
			enum gfx_texture_access_type src_access = gfx_texture_get_access_type(texture,
											     i, j, 0);

			if (src_access == dst_access)
				continue;
			
			gfx_texture_set_access_type(texture,
						    i, j, 0,
						    dst_access);
		
			struct gfx_texture_access src_access_info = gfx_sync_get_src_texture_access(src_access);

			barriers[*barrier_count] = gfx_sync_texture_memory_barrier(texture,
										   src_access_info,
										   dst_access_info,
										   i, 1,
										   j, 1);
			
			*barrier_count = *barrier_count + 1;
		}
	}
}

static void gfx_render_stage_transition_view(struct gfx_texture_view *view,
					     enum gfx_texture_access_type dst_access,
					     VkImageMemoryBarrier2 *barriers, u32 *barrier_count)
{
	struct gfx_texture_access dst_access_info = gfx_sync_get_dst_texture_access(dst_access);

	for (u32 j = 0; j < view->layer_count; j++) {
		enum gfx_texture_access_type curr_src_access = gfx_texture_get_access_type(view->parent,
											   view->base_level,
											   view->base_layer + j, 0);
			
		gfx_texture_set_access_type(view->parent,
					    view->base_level,
					    view->base_layer + j, 0,
					    dst_access);

		u32 chain_length = 1;
		u32 curr_mip = 0;
		
		for (u32 i = 1; i < view->level_count; i++) {
			enum gfx_texture_access_type new_src_access = gfx_texture_get_access_type(view->parent,
												  view->base_level + i,
												  view->base_layer + j, 0);
			
			gfx_texture_set_access_type(view->parent,
						    view->base_level + i,
						    view->base_layer + j, 0,
						    dst_access);

			if (curr_src_access == new_src_access) {
				// Continue the chain.
				chain_length++;
			} else {
				// Generate a new pipeline barrier.
				curr_src_access = new_src_access;
				
				barriers[*barrier_count] = gfx_sync_texture_memory_barrier(view->parent,
											   gfx_sync_get_src_texture_access(curr_src_access),
											   dst_access_info,
											   view->base_level + curr_mip, chain_length,
											   view->base_layer + j, 1);
			
				*barrier_count = *barrier_count + 1;

				chain_length = 1;
				curr_mip = i;
			}
		}
				
		barriers[*barrier_count] = gfx_sync_texture_memory_barrier(view->parent,
									   gfx_sync_get_src_texture_access(curr_src_access),
									   dst_access_info,
									   view->base_level + curr_mip, chain_length,
									   view->base_layer + j, 1);
			
		*barrier_count = *barrier_count + 1;
	}
}

static void gfx_render_stage_transition_buffer(struct gfx_buffer *buffer,
					       enum gfx_buffer_access_type dst_access,
					       VkBufferMemoryBarrier2 *barriers, u32 *barrier_count)
{
	struct gfx_buffer_access src_access_info = gfx_sync_get_src_buffer_access(buffer->access_type);
	struct gfx_buffer_access dst_access_info = gfx_sync_get_dst_buffer_access(dst_access);

	buffer->access_type = dst_access;

	barriers[*barrier_count] = gfx_sync_buffer_memory_barrier(buffer, src_access_info, dst_access_info);
	*barrier_count = *barrier_count + 1;
}

void gfx_render_stage_init(struct gfx_render_stage *stage, enum gfx_render_stage_type type)
{
	stage->type = type;
}

void gfx_render_stage_execute(struct gfx_render_stage *stage, struct gfx_device *device, struct gfx_command_buffer *cmd, struct gfx_swapchain *swapchain)
{
	struct scratch_arena scratch = scratch_begin(NULL, 0);
	
	u32 image_barrier_count = 0;
	u32 buffer_barrier_count = 0;

	// TODO: Convert these into linked lists.
	VkImageMemoryBarrier2 *image_barriers = memory_arena_array(scratch.arena, 128, sizeof(VkImageMemoryBarrier2));
	VkBufferMemoryBarrier2 *buffer_barriers = memory_arena_array(scratch.arena, stage->buffer_count, sizeof(VkBufferMemoryBarrier2));

	for (int i = 0; i < stage->attachment_count; i++)
		gfx_render_stage_transition_view(stage->attachments[i].view,
						 stage->attachments[i].view->parent->is_depth ? GFX_TEXTURE_ACCESS_TYPE_depth : GFX_TEXTURE_ACCESS_TYPE_colour,
						 image_barriers, &image_barrier_count);

	for (int i = 0; i < stage->view_count; i++)
		gfx_render_stage_transition_view(stage->views[i].view,
						 stage->views[i].access_type,
						 image_barriers, &image_barrier_count);
	
	for (int i = 0; i < stage->buffer_count; i++)
		gfx_render_stage_transition_buffer(stage->buffers[i].buffer,
						   stage->buffers[i].access_type,
						   buffer_barriers, &buffer_barrier_count);

	switch (stage->type) {
	case GFX_RENDER_STAGE_mipmap:
		gfx_render_stage_transition_texture(stage->mipmap_texture,
						    GFX_TEXTURE_ACCESS_TYPE_blit_dst,
						    image_barriers, &image_barrier_count);
		break;
	case GFX_RENDER_STAGE_present:
		gfx_render_stage_transition_texture(gfx_swapchain_current_texture(swapchain),
						    GFX_TEXTURE_ACCESS_TYPE_present,
						    image_barriers, &image_barrier_count);
		break;
	}

	gfx_cmd_pipeline_barrier(cmd, 0,
				 0, NULL,
				 buffer_barrier_count, buffer_barriers,
				 image_barrier_count, image_barriers);
	
	scratch_release(&scratch);
	
	struct gfx_render_state render_state = {0};
	render_state.device = device;
	render_state.cmd = cmd;
	render_state.view = stage->render_view;

	switch (stage->type) {
	case GFX_RENDER_STAGE_graphics: {
		struct gfx_render_info render_info = {0};
		render_info.view_mask = stage->graphics_view_mask;

		for (int i = 0; i < stage->attachment_count; i++) {
			struct gfx_render_stage_attachment *attachment = stage->attachments + i;
			
			render_info.samples = attachment->view->parent->samples;
			
			gfx_render_stage_attachment_get_absolute_size(attachment, swapchain,
								      &render_info.width,
								      &render_info.height);

			VkRenderingAttachmentInfo attachment_info = vk_rendering_attachment_from_stage_attachment(attachment);

			if (attachment->view->parent->is_depth) {
				render_info.depth_attachment = attachment_info;
			} else {
				render_info.colour_attachments[render_info.colour_attachment_count] = attachment_info;
				render_info.colour_attachment_count++;
			}
		}
		
		gfx_cmd_begin_rendering(cmd, &render_info);
		break;
	}
	case GFX_RENDER_STAGE_mipmap:
		gfx_cmd_generate_mipmaps(cmd, stage->mipmap_texture);
		break;
	}
	
	for (int i = 0; i < stage->feature_count; i++)
		stage->features[i].record(stage->features[i].self, &render_state);
	
	switch (stage->type) {
	case GFX_RENDER_STAGE_graphics:
		gfx_cmd_end_rendering(cmd);
		break;
	}
}

void gfx_render_stage_load_resources(struct gfx_render_stage *stage, struct gfx_device *device, struct gfx_resource_database *db)
{
}

void gfx_render_stage_resize(struct gfx_render_stage *stage, struct gfx_device *device, u32 width, u32 height)
{
}

void gfx_render_stage_set_render_view(struct gfx_render_stage *stage,
				      struct gfx_render_view *view)
{
	stage->render_view = view;
}

void gfx_render_stage_set_graphics_view_mask(struct gfx_render_stage *stage,
					     u32 view_mask)
{
	stage->graphics_view_mask = view_mask;
}

void gfx_render_stage_add_feature(struct gfx_render_stage *stage,
				  u64 self_size, void *self,
				  gfx_render_feature_record_t record)
{
	assert(stage->feature_count < array_size(stage->features));
	assert(self_size <= GFX_RENDER_FEATURE_MAX_CONTEXT_BYTES);

	memory_copy(stage->features[stage->feature_count].self, self, self_size);
	stage->features[stage->feature_count].record = record;

	stage->feature_count++;
}

void gfx_render_stage_add_attachment(struct gfx_render_stage *stage,
				     const struct gfx_render_stage_attachment *attachment)
{
	assert(stage->attachment_count < array_size(stage->attachments));
	
	stage->attachments[stage->attachment_count++] = *attachment;
}

void gfx_render_stage_add_view(struct gfx_render_stage *stage,
			       struct gfx_texture_view *view,
			       enum gfx_texture_access_type access_type)
{
	assert(stage->view_count < array_size(stage->views));

	stage->views[stage->view_count].view = view;
	stage->views[stage->view_count].access_type = access_type;

	stage->view_count++;
}

void gfx_render_stage_add_buffer(struct gfx_render_stage *stage,
				 struct gfx_buffer *buffer,
				 enum gfx_buffer_access_type access_type)
{
	assert(stage->buffer_count < array_size(stage->buffers));

	stage->buffers[stage->buffer_count].buffer = buffer;
	stage->buffers[stage->buffer_count].access_type = access_type;

	stage->buffer_count++;
}

void gfx_render_graph_init(struct gfx_render_graph *graph)
{
	gfx_render_graph_reset(graph);
}

void gfx_render_graph_reset(struct gfx_render_graph *graph)
{
	graph->stage_count = 0;
}

void gfx_render_graph_destroy(struct gfx_render_graph *graph)
{
}

void gfx_render_graph_update(struct gfx_render_graph *graph)
{
}

void gfx_render_graph_render(struct gfx_render_graph *graph,
			     struct gfx_device *device,
			     struct gfx_swapchain *swapchain,
			     struct gfx_command_buffer *cmd)
{
	for (int i = 0; i < graph->stage_count; i++)
		gfx_render_stage_execute(graph->stages + i, device, cmd, swapchain);
}

void gfx_render_graph_load_resources(struct gfx_render_graph *graph, struct gfx_device *device)
{
	for (int i = 0; i < graph->stage_count; i++)
		gfx_render_stage_load_resources(&graph->stages[i], device, &graph->resource_database);
}

void gfx_render_graph_resize(struct gfx_render_graph *graph, struct gfx_device *device, u32 width, u32 height)
{
	for (int i = 0; i < graph->stage_count; i++)
		gfx_render_stage_resize(&graph->stages[i], device, width, height);
}

void gfx_render_graph_push(struct gfx_render_graph *graph, struct gfx_render_stage *stage)
{
	assert(graph->stage_count < array_size(graph->stages));
	graph->stages[graph->stage_count++] = *stage;
}
