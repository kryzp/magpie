#include "stage.h"

#include "app.h"

static void hdr_texture_to_cubemap_feature(void *self, struct gfx_render_state *rs)
{
	struct stage_hdr_texture_to_cubemap_input *context = self;
	struct gfx_command_buffer *cmd = rs->cmd;

	struct {
		u64 transform_matrix_buffer;
		u64 vertex_buffer;
		u32 hdr_image_id;
		u32 linear_sampler_id;
	} args;

	args.transform_matrix_buffer = app->cubemap_capture_transforms.device_address;
	args.vertex_buffer = app->skybox_mesh.vertex_buffer.device_address;
	args.hdr_image_id = context->texture->bindless.sampled;
	args.linear_sampler_id = app->linear_sampler.bindless.id;

	struct gfx_graphics_pipeline_def pipeline_def = gfx_graphics_pipeline_def_init(&app->shaders.hdr_to_environment_cubemap_program);
	pipeline_def.depth_stencil_state.depth_test_enabled = false;
	pipeline_def.depth_stencil_state.depth_write_enabled = false;
	pipeline_def.colour_attachment_count = 1;
	pipeline_def.colour_attachment_formats[0] = VK_FORMAT_R32G32B32A32_SFLOAT;
	pipeline_def.view_mask = 0b111111;

	struct gfx_pipeline_st st = gfx_device_pipeline_fetch_graphics(rs->device, &pipeline_def);

	gfx_cmd_bind_bindless(cmd, st.bind_point, st.layout, rs->device);
	gfx_cmd_bind_pipeline(cmd, st.bind_point, st.pipeline);

	gfx_cmd_push_constants(cmd, st.layout, VK_SHADER_STAGE_ALL_GRAPHICS, sizeof(args), &args);

	gfx_mesh_bind_indices(&app->skybox_mesh, cmd);
	gfx_mesh_draw_indexed(&app->skybox_mesh, cmd);
}

void stage_add_hdr_texture_to_cubemap(struct gfx_render_graph *graph,
				      struct stage_hdr_texture_to_cubemap_input *input)
{
	struct gfx_render_stage stage = {0};
	gfx_render_stage_init(&stage, GFX_RENDER_STAGE_graphics);

	gfx_render_stage_add_feature(&stage,
				     sizeof(struct stage_hdr_texture_to_cubemap_input), input,
				     hdr_texture_to_cubemap_feature);

	gfx_render_stage_set_graphics_view_mask(&stage, 0b111111);

	gfx_render_stage_add_view(&stage, input->texture, GFX_TEXTURE_ACCESS_TYPE_graphics_r);

	struct gfx_render_stage_attachment attachment = {0};

	gfx_render_stage_attachment_init_colour(&attachment,
						input->output,
						GFX_RENDER_SIZE_absolute,
						true, v4(0.f, 0.f, 0.f, 1.f));
	
	gfx_render_stage_add_attachment(&stage, &attachment);

		
	struct gfx_render_stage mipmapping = {0};
	gfx_render_stage_init(&mipmapping, GFX_RENDER_STAGE_mipmap);
	mipmapping.mipmap_texture = input->output->parent;

	gfx_render_graph_push(graph, &stage);
	gfx_render_graph_push(graph, &mipmapping);
}
