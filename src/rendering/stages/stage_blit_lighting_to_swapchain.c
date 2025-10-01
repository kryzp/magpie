#include "stage.h"

static void blit_lighting_to_swapchain_feature(void *self, struct gfx_render_state *rs)
{
	struct stage_blit_lighting_to_swapchain_input *context = self;
	
	struct gfx_texture_view *src = context->lighting;
	struct gfx_texture_view *dst = context->swapchain;
	
	VkImageBlit2 region = {0};

	region.sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2;
	
	region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.srcSubresource.mipLevel = src->base_level;
	region.srcSubresource.baseArrayLayer = src->base_layer;
	region.srcSubresource.layerCount = src->layer_count;
	region.srcOffsets[0] = (VkOffset3D){ 0, 0, 0 };
	region.srcOffsets[1] = (VkOffset3D){ src->parent->width, src->parent->height, 1 };

	region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.dstSubresource.mipLevel = dst->base_level;
	region.dstSubresource.baseArrayLayer = dst->base_layer;
	region.dstSubresource.layerCount = dst->layer_count;
	region.dstOffsets[0] = (VkOffset3D){ 0, 0, 0 };
	region.dstOffsets[1] = (VkOffset3D){ dst->parent->width, dst->parent->height, 1 };

	gfx_cmd_blit(rs->cmd,
		     src->parent, dst->parent,
		     1, &region,
		     VK_FILTER_LINEAR);
	
}

void stage_add_blit_lighting_to_swapchain(struct gfx_render_graph *graph,
					  struct stage_blit_lighting_to_swapchain_input *input)
{
	struct gfx_render_stage stage = {0};
	gfx_render_stage_init(&stage, GFX_RENDER_STAGE_transfer);

	gfx_render_stage_add_feature(&stage,
				     sizeof(struct stage_blit_lighting_to_swapchain_input), input,
				     blit_lighting_to_swapchain_feature);
	
	gfx_render_stage_add_view(&stage, input->lighting, GFX_TEXTURE_ACCESS_TYPE_blit_src);
	gfx_render_stage_add_view(&stage, input->swapchain, GFX_TEXTURE_ACCESS_TYPE_blit_dst);
	
	gfx_render_graph_push(graph, &stage);
}
