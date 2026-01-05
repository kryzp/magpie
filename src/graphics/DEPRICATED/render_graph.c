#include "render_graph.h"

#include "core/core_scratch.h"

#include "sync.h"

static void attachment_get_absolute_size(struct gfx_render_attachment *attachment,
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

static struct gfx_texture *create_texture_from_attachment(struct gfx_render_attachment *attachment,
							  struct gfx_device *device,
							  struct gfx_swapchain *swapchain)
{
	struct gfx_texture *texture = NULL;

	u32 w, h;
	attachment_get_absolute_size(attachment, swapchain, &w, &h);
	
	if (attachment->is_depth_stencil) {
	} else {
		texture = gfx_device_texture_alloc(device,
						   w, h, 1,
						   attachment->colour_format,
						   VK_IMAGE_VIEW_TYPE_2D,
						   VK_IMAGE_TILING_OPTIMAL,
						   1);
	}

	return texture;
}

void gfx_render_attachment_init_colour(struct gfx_render_attachment *attachment,
				       struct string8 name,
				       enum gfx_render_size_class size_class,
				       VkFormat format, float size_x, float size_y)
{
	assert(name.len <= array_size(attachment->name));
	
	memory_copy(attachment->name, name.str, name.len);
	
	attachment->is_depth_stencil = false;
	attachment->colour_format = format;
	attachment->size_class = size_class;
	attachment->size_x = size_x;
	attachment->size_y = size_y;

	/*
	attachment->size_x = view->parent->width >> view->base_level;
	attachment->size_y = view->parent->height >> view->base_level;
	*/
}

void gfx_render_attachment_init_depth_stencil(struct gfx_render_attachment *attachment,
						    struct string8 name,
						    enum gfx_render_size_class size_class,
						    float size_x, float size_y)
{
	assert(name.len <= array_size(attachment->name));
	
	memory_copy(attachment->name, name.str, name.len);
	
	attachment->is_depth_stencil = true;
	attachment->colour_format = VK_FORMAT_UNDEFINED; 
	attachment->size_class = size_class;
	attachment->size_x = size_x;
	attachment->size_y = size_y;

	/*
	attachment->size_x = view->parent->width >> view->base_level;
	attachment->size_y = view->parent->height >> view->base_level;
	*/
}

static VkRenderingAttachmentInfo vk_rendering_attachment_from_attachment(struct gfx_render_attachment *attachment)
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
		for (u32 j = 0; j < texture->layer_count; j++) {
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
	/*
	 * Could this be optimized with a flood-fill style algorithm?
	 * Right now it just moves horizontally.
	 */
	
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

void gfx_render_stage_execute(struct gfx_render_stage *stage, struct gfx_device *device, struct gfx_command_buffer *cmd, struct gfx_swapchain *swapchain, struct gfx_render_graph *graph)
{
	struct scratch_arena scratch = scratch_begin(NULL, 0);
	
	u32 image_barrier_count = 0;
	u32 buffer_barrier_count = 0;

	// TODO: Convert these into linked lists.
	VkImageMemoryBarrier2 *image_barriers = memory_arena_array(scratch.arena, 128, sizeof(VkImageMemoryBarrier2));
	VkBufferMemoryBarrier2 *buffer_barriers = memory_arena_array(scratch.arena, stage->buffer_count, sizeof(VkBufferMemoryBarrier2));

	for (int i = 0; i < stage->attachment_count; i++) {
		struct gfx_texture_view *view = gfx_render_graph_
		gfx_render_stage_transition_view(stage->attachments[i].view,
						 stage->attachments[i].view->parent->is_depth ? GFX_TEXTURE_ACCESS_TYPE_depth : GFX_TEXTURE_ACCESS_TYPE_colour,
						 image_barriers, &image_barrier_count);
	}
	
	for (int i = 0; i < stage->texture_view_count; i++)
		gfx_render_stage_transition_view(stage->texture_views[i].view,
						 stage->texture_views[i].access_type,
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
	
	struct gfx_render_context render_context = {0};
	render_context.device = device;
	render_context.cmd = cmd;
	render_context.view = &stage->render_view;
	render_context.attachments = &graph->attachment_textures;

	switch (stage->type) {
	case GFX_RENDER_STAGE_graphics: {
		struct gfx_render_info render_info = {0};
		render_info.view_mask = stage->multi_view_mask;

		for (int i = 0; i < stage->attachment_count; i++) {
			struct gfx_render_attachment *attachment = stage->attachments + i;

			struct gfx_texture *resolved_texture = hash_table_fetch(&graph->attachment_textures, hash_str8(attachment->name); 

			if (!resolved_texture) {
				resolved_texture = ;
				hash_table_add(&graph->attachment_textures, hash_str8(attachment->name));
			}
			
			// TODO: Right now it's just based on the last attachments sample count.
			//       Assumption is that all attachments will already have the same sample count.
			//       Ideally I should have resolving implemented so they automatically have their resolves.
			render_info.samples = attachment->view->parent->samples;
			
			attachment_get_absolute_size(attachment, swapchain,
						     &render_info.width,
						     &render_info.height);

			VkRenderingAttachmentInfo attachment_info = vk_rendering_attachment_from_attachment(attachment);

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
	
	for (int i = 0; i < stage->feature_renderer_count; i++)
		stage->feature_renderers[i].record(stage->feature_renderers[i].state, &render_context);
	
	switch (stage->type) {
	case GFX_RENDER_STAGE_graphics:
		gfx_cmd_end_rendering(cmd);
		break;
	}
}

void gfx_render_stage_set_render_view(struct gfx_render_stage *stage,
				      struct gfx_render_view view)
{
	stage->render_view = view;
}

void gfx_render_stage_set_multi_view_mask(struct gfx_render_stage *stage,
					  u32 view_mask)
{
	stage->multi_view_mask = view_mask;
}

void gfx_render_stage_add_feature_renderer(struct gfx_render_stage *stage,
					   void *state, gfx_render_feature_record_t record)
{
	assert(stage->feature_renderer_count < array_size(stage->feature_renderers));

	stage->feature_renderers[stage->feature_renderer_count].state = state;
	stage->feature_renderers[stage->feature_renderer_count].record = record;

	stage->feature_renderer_count++;
}

void gfx_render_stage_add_attachment(struct gfx_render_stage *stage,
				     struct string8 name,
				     struct gfx_render_clear *clear)
{
	assert(stage->attachment_count < array_size(stage->attachments));
	
	stage->attachments[stage->attachment_count].name = name;
	stage->attachments[stage->attachment_count].clear = *clear;
	
	stage->attachment_count++;
}

void gfx_render_stage_add_texture_view(struct gfx_render_stage *stage,
				       struct gfx_texture_view *view,
				       enum gfx_texture_access_type access_type)
{
	assert(stage->texture_view_count < array_size(stage->texture_views));

	stage->texture_views[stage->texture_view_count].view = view;
	stage->texture_views[stage->texture_view_count].access_type = access_type;

	stage->texture_view_count++;
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
}

void gfx_render_graph_destroy(struct gfx_render_graph *graph, struct gfx_device *device)
{
	for (int i = 0; i < graph->attachment_count; i++)
		gfx_device_texture_destroy(device, graph->attachments[i].texture);

	graph->attachment_count = 0;
}

void gfx_render_graph_attach(struct gfx_render_graph *graph,
			     struct string8 name,
			     struct gfx_render_stage_attachment *attachment)
{
	assert(graph->attachment_count < array_size(graph->attachments));
	
	graph->attachments[graph->attachment_count].name = name;
	graph->attachments[graph->attachment_count].attachment = *attachment;
	
	graph->attachment_count++;
}

void gfx_render_graph_update(struct gfx_render_graph *graph)
{
}

void gfx_render_graph_render(struct gfx_render_graph *graph,
			     struct gfx_device *device,
			     struct gfx_swapchain *swapchain,
			     struct gfx_command_buffer *cmd)
{
	for (int i = 0; i < graph->attachment_count; i++) {
		if (!graph->attachments[i].texture)
			graph->attachments[i].texture = create_texture_from_attachment(&graph->attachments[i].attachment,
										       device,
										       swapchain);
	}
	
	for (int i = 0; i < graph->stage_count; i++)
		gfx_render_stage_execute(graph->stages + i, device, cmd, swapchain);
}

void gfx_render_graph_resize(struct gfx_render_graph *graph, struct gfx_device *device, u32 width, u32 height)
{
	for (int i = 0; i < graph->attachment_count; i++) {
		// TODO
	}
}

void gfx_render_graph_push(struct gfx_render_graph *graph, struct gfx_render_stage *stage)
{
	assert(graph->stage_count < array_size(graph->stages));
	graph->stages[graph->stage_count++] = *stage;
}
