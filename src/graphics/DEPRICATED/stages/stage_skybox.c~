#include "stage.h"

#include "../device.h"

#include "app.h"

static void skybox_feature(void *self, struct gfx_render_state *rs)
{
	struct stage_skybox_input *context = self;
	struct gfx_command_buffer *cmd = rs->cmd;

	struct gfx_graphics_pipeline_def pipeline_def = gfx_graphics_pipeline_def_init(&app->shaders.skybox_program);
	pipeline_def.has_depth_attachment = true;
	pipeline_def.depth_stencil_state.depth_test_enabled = true;
	pipeline_def.depth_stencil_state.depth_write_enabled = false;
	pipeline_def.depth_stencil_state.depth_compare_op = VK_COMPARE_OP_LESS_OR_EQUAL;
	pipeline_def.colour_attachment_count = 1;
	pipeline_def.colour_attachment_formats[0] = context->target->parent->format;

	struct gfx_pipeline_st st = gfx_device_pipeline_fetch_graphics(rs->device, &pipeline_def);

	struct {
		u64 frame_data_buffer;
		u64 vertex_buffer;
		u32 cubemap_id;
		u32 sampler_id;
	} args;

	args.frame_data_buffer = context->frame_data_buffer->device_address;
	args.vertex_buffer = app->skybox_mesh.vertex_buffer.device_address;
	args.cubemap_id = context->skybox->bindless.sampled;
	args.sampler_id = app->linear_sampler.bindless.id;

	gfx_cmd_bind_bindless(cmd, st.bind_point, st.layout, rs->device);
	gfx_cmd_bind_pipeline(cmd, st.bind_point, st.pipeline);

	gfx_cmd_push_constants(cmd, st.layout, VK_SHADER_STAGE_ALL_GRAPHICS, sizeof(args), &args);

	gfx_mesh_bind_indices(&app->skybox_mesh, cmd);
	gfx_mesh_draw_indexed(&app->skybox_mesh, cmd);
}

void stage_add_skybox(struct gfx_render_graph *graph,
		      struct stage_skybox_input *input)
{
	struct gfx_render_stage stage = {0};
	gfx_render_stage_init(&stage, GFX_RENDER_STAGE_graphics);

	gfx_render_stage_add_feature(&stage,
				     sizeof(struct stage_skybox_input), input,
				     skybox_feature);

	gfx_render_stage_add_view(&stage, input->skybox, GFX_TEXTURE_ACCESS_TYPE_graphics_r);

	struct gfx_render_stage_attachment colour_attachment = {0};
	struct gfx_render_stage_attachment depth_attachment = {0};

	gfx_render_stage_attachment_init_colour(&colour_attachment,
						input->target,
						GFX_RENDER_SIZE_absolute,
						true, v4(0.f, 0.f, 0.f, 1.f));

	gfx_render_stage_attachment_init_depth_stencil(&depth_attachment,
						       input->depth,
						       GFX_RENDER_SIZE_absolute,
						       true, 1.f, 0);

	gfx_render_stage_add_attachment(&stage, &colour_attachment);
	gfx_render_stage_add_attachment(&stage, &depth_attachment);

	gfx_render_graph_push(graph, &stage);
}
