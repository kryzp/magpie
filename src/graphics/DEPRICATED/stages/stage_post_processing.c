#include "stage.h"

#include "../device.h"

#include "app.h"

static void post_processing_feature(void *self, struct gfx_render_state *rs)
{
	struct stage_post_processing_input *context = self;
	struct gfx_command_buffer *cmd = rs->cmd;
	
	struct gfx_compute_pipeline_def pipeline_def = gfx_compute_pipeline_def_init(&app->shaders.hdr_tonemapping_program);
	struct gfx_pipeline_st pipeline_st = gfx_device_pipeline_fetch_compute(rs->device, &pipeline_def);

	gfx_cmd_bind_bindless(cmd, pipeline_st.bind_point, pipeline_st.layout, rs->device);
	gfx_cmd_bind_pipeline(cmd, pipeline_st.bind_point, pipeline_st.pipeline);

	struct {
		u32 width;
		u32 height;
		float exposure;
		u32 input_image_id;
		u32 output_image_id;
	} args;

	args.width = context->input->parent->width;
	args.height = context->input->parent->height;
	args.exposure = context->exposure;
	args.input_image_id = context->input->bindless.sampled;
	args.output_image_id = context->output->bindless.storage;
	
	gfx_cmd_push_constants(cmd,
			       pipeline_st.layout,
			       VK_SHADER_STAGE_COMPUTE_BIT,
			       sizeof(args), &args);

	gfx_cmd_dispatch(cmd,
			 context->output->parent->width,
			 context->output->parent->height, 1);
}

void stage_add_post_processing(struct gfx_render_graph *graph,
			       struct stage_post_processing_input *input)
{
	struct gfx_render_stage stage = {0};
	gfx_render_stage_init(&stage, GFX_RENDER_STAGE_compute);

	gfx_render_stage_add_feature(&stage,
				     sizeof(struct stage_post_processing_input), input,
				     post_processing_feature);
	
	gfx_render_stage_add_view(&stage, input->input, GFX_TEXTURE_ACCESS_TYPE_compute_r);
	gfx_render_stage_add_view(&stage, input->output, GFX_TEXTURE_ACCESS_TYPE_compute_rw);
	
	gfx_render_graph_push(graph, &stage);
}
