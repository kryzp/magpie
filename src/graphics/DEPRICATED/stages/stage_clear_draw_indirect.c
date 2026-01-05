#include "stage.h"

#include "../gpu_types.h"

static void clear_draw_indirect_feature(void *self, struct gfx_render_state *rs)
{
	/*
	struct stage_clear_draw_indirect_input *context = self;
	
	VkBufferCopy indirect_region = {0};
	indirect_region.srcOffset = 0;
	indirect_region.dstOffset = 0;
	indirect_region.size = context->batch_count * sizeof(struct gfx_gpu_indirect);
	
	gfx_cmd_copy_buffer_to_buffer(rs->cmd,
				      context->clear_indirect_buffer,
				      context->draw_indirect_buffer,
				      1, &indirect_region);
	*/	
}

void stage_add_clear_draw_indirect(struct gfx_render_graph *graph,
				   struct stage_clear_draw_indirect_input *input)
{
	struct gfx_render_stage stage = {0};
	gfx_render_stage_init(&stage, GFX_RENDER_STAGE_transfer);

	gfx_render_stage_add_feature(&stage,
				     sizeof(struct stage_clear_draw_indirect_input), input,
				     clear_draw_indirect_feature);
	
	gfx_render_stage_add_buffer(&stage, input->clear_indirect_buffer, GFX_BUFFER_ACCESS_TYPE_copy_src);
	gfx_render_stage_add_buffer(&stage, input->draw_indirect_buffer, GFX_BUFFER_ACCESS_TYPE_copy_dst);
	
	gfx_render_graph_push(graph, &stage);
}
