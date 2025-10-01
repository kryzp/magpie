#include "stage.h"

#include "app.h"

static void geometry_stage_feature(void *self, struct gfx_render_state *rs)
{
	struct geometry_pass_context *context = self;
	struct gfx_command_buffer *cmd = rs->cmd;

	struct gfx_graphics_pipeline_def pipeline_def = gfx_graphics_pipeline_def_init(&shaders->model_program);
	pipeline_def.has_depth_attachment = true;
	pipeline_def.colour_attachment_count = GFX_GBUFFER_ATTACHMENT_max_enum;
	
	for (int i = 0; i < GFX_GBUFFER_ATTACHMENT_max_enum; i++)
		pipeline_def.colour_attachment_formats[i] = pass_context->gbuffer->attachments[i].format;

	struct gfx_pipeline_st pipeline_st = FetchGraphicsPipeline(&pipeline_def);

	gfx_cmd_bind_bindless(cmd, pipeline_st.bind_point, pipeline_st.layout);
	gfx_cmd_bind_pipeline(cmd, pipeline_st.bind_point, pipeline_st.pipeline);

	struct gfx_mesh_pass *pass = &rs->view->mesh_passes[GFX_MESH_PASS_forward];

	for (int i = 0; i < pass->multi_batch_count; i++) {
		struct gfx_multi_batch *multi_batch = pass->multi_batches + i;
		struct gfx_indirect_batch *batch = pass->batches + multi_batch->first;
		
		struct {
			u64 frame_data_buffer;
			u64 transform_buffer;
			u64 material_buffer;
			u64 vertex_buffer;
			u64 instance_buffer;
			u32 material_id;
			u32 sampler;
		} args;
		
		// TODO: material_id should be inferred by indexing into a gpu buffer
		//       with instance_id, rather than being given by push constants.
		
		args.frame_data_buffer = pass_context->frame_data_buffer->device_address;
		args.transform_buffer = pass_context->object_buffer->device_address;
		args.material_buffer = rs->material_buffer->device_address;
		args.vertex_buffer = rs->meshes[batch->mesh_id].original->vertex_buffer.device_address;
		args.instance_buffer = pass_context->instance_buffer->device_address;
		args.material_id = batch->material_id;
		args.sampler = app->linear_sampler.bindless.id;

		gfx_cmd_push_constants(cmd,
				       pipeline_st.layout,
				       VK_SHADER_STAGE_ALL_GRAPHICS,
				       sizeof(args), &args);

		gfx_mesh_bind_indices(rs->meshes[batch->mesh_id].original, cmd);

		gfx_cmd_draw_indexed_indirect(cmd, pass_context->indirect_buffer,
					      multi_batch->first * sizeof(struct gfx_gpu_indirect),
					      multi_batch->count,
					      sizeof(struct gfx_gpu_indirect));
	}
}

static void lighting_stage_feature(void *self, struct gfx_render_state *rs)
{
	struct lighting_pass_context *context = self;
	struct gfx_command_buffer *cmd = rs->cmd;

	struct gfx_gbuffer *gbuffer = pass_context->gbuffer;
	struct gfx_environment_probe *probe = pass_context->probe;

	struct gfx_pipeline_st pipeline_st = {0};
	
	// Ambient Lighting.
	struct gfx_graphics_pipeline_def ambient_pipeline_def = gfx_graphics_pipeline_def_init(&shaders->ambient_lighting_program);
	ambient_pipeline_def.depth_stencil_state.depth_test_enabled = false;
	ambient_pipeline_def.depth_stencil_state.depth_write_enabled = false;
	ambient_pipeline_def.colour_attachment_count = 1;
	ambient_pipeline_def.colour_attachment_formats[0] = pass_context->target->image->format;
	
	pipeline_st = gfx_device_pipeline_fetch_graphics(rs->device, &ambient_pipeline_def);

	gfx_cmd_bind_bindless(cmd, pipeline_st.bind_point, pipeline_st.layout);
	gfx_cmd_bind_pipeline(cmd, pipeline_st.bind_point, pipeline_st.pipeline);

	struct {
		u64 frame_data_buffer;
		u32 position;
		u32 albedo;
		u32 normal;
		u32 material;
		u32 emissive;
		u32 irradiance_map;
		u32 prefilter_map;
		u32 brdf_lut;
		u32 linear_sampler;
	} pc_ambient;

	pc_ambient.frame_data_buffer = pass_context->frame_data_buffer->device_address;
	pc_ambient.position = gbuffer->views[GFX_GBUFFER_ATTACHMENT_position]              ->bindless.sampled;
	pc_ambient.albedo   = gbuffer->views[GFX_GBUFFER_ATTACHMENT_albedo]                ->bindless.sampled;
	pc_ambient.normal   = gbuffer->views[GFX_GBUFFER_ATTACHMENT_normal]                ->bindless.sampled;
	pc_ambient.material = gbuffer->views[GFX_GBUFFER_ATTACHMENT_metallic_roughness]    ->bindless.sampled;
	pc_ambient.emissive = gbuffer->views[GFX_GBUFFER_ATTACHMENT_emissive]              ->bindless.sampled;
	pc_ambient.irradiance_map = gfx_device_texture_view_fetch_std(&probe->irradiance)  ->bindless.sampled;
	pc_ambient.prefilter_map  = gfx_device_texture_view_fetch_std(&probe->prefilter)   ->bindless.sampled;
	pc_ambient.brdf_lut = gfx_device_texture_view_fetch_std(&app->brdf_lut_image)     ->bindless.sampled;
	pc_ambient.linear_sampler = app->linear_sampler.bindless.id;

	gfx_cmd_push_constants(cmd, pipeline_st.layout, VK_SHADER_STAGE_ALL_GRAPHICS, sizeof(pc_ambient), &pc_ambient);
	gfx_cmd_draw_vertices_n(cmd, 3);

	// Direct lighting.
	struct gfx_graphics_pipeline_def direct_pipeline_def = GraphicsPipelineDefInitDefault(&shaders->direct_lighting_point_program);
	direct_pipeline_def.depth_stencil_state.depth_test_enabled = false;
	direct_pipeline_def.depth_stencil_state.depth_write_enabled = false;
	direct_pipeline_def.cull_mode = VK_CULL_MODE_FRONT_BIT;
	direct_pipeline_def.colour_attachment_count = 1;
	direct_pipeline_def.colour_attachment_formats[0] = pass_context->target->image->format;
	direct_pipeline_def.blend_state.enabled = true;
	direct_pipeline_def.blend_state.colour.op = VK_BLEND_OP_ADD;
	direct_pipeline_def.blend_state.colour.dst = VK_BLEND_FACTOR_ONE;
	direct_pipeline_def.blend_state.colour.src = VK_BLEND_FACTOR_ONE;

	pipeline_st = gfx_device_pipeline_fetch_graphics(&direct_pipeline_def);

	gfx_cmd_bind_bindless(cmd, pipeline_st.bind_point, pipeline_st.layout);
	gfx_cmd_bind_pipeline(cmd, pipeline_st.bind_point, pipeline_st.pipeline);

	gfx_mesh_bind_indices(&app->light_sphere_mesh, cmd);

	// TODO: Make this into instanced rendering?
	for (int i = 0; i < rs->view->light_count; i++) {
		struct {
			u64 frame_data_buffer;
			u64 light_buffer;
			u64 vertex_buffer;
			u32 position;
			u32 albedo;
			u32 normal;
			u32 material;
			u32 emissive;
			u32 linear_sampler;
		} pc_direct;

		pc_direct.frame_data_buffer = pass_context->frame_data_buffer        ->device_address;
		pc_direct.light_buffer      = pass_context->light_buffer             ->device_address;
		pc_direct.vertex_buffer     = app->light_sphere_mesh.vertex_buffer    .device_address;
				
		pc_direct.position = gbuffer->views[GFX_GBUFFER_ATTACHMENT_position]            ->bindless.sampled;
		pc_direct.albedo   = gbuffer->views[GFX_GBUFFER_ATTACHMENT_albedo]              ->bindless.sampled;
		pc_direct.normal   = gbuffer->views[GFX_GBUFFER_ATTACHMENT_normal]              ->bindless.sampled;
		pc_direct.material = gbuffer->views[GFX_GBUFFER_ATTACHMENT_metallic_roughness]  ->bindless.sampled;
		pc_direct.emissive = gbuffer->views[GFX_GBUFFER_ATTACHMENT_emissive]            ->bindless.sampled;
				
		pc_direct.linear_sampler = app->linear_sampler.bindless.id;

		gfx_cmd_push_constants(cmd,
				       pipeline_st.layout,
				       VK_SHADER_STAGE_ALL_GRAPHICS,
				       sizeof(pc_direct), &pc_direct);

		gfx_mesh_draw_indexed_id(&app->light_sphere_mesh, cmd, i);
	}
}

void stage_add_deferred(struct gfx_render_graph *graph,
			struct stage_deferred_input *input)
{
	struct gfx_render_stage geometry_stage = {0};
	gfx_render_stage_init(&geometry_stage, GFX_RENDER_STAGE_graphics);

	gfx_render_stage_add_feature(&geometry_stage,
				     sizeof(struct stage_deferred_input), input,
				     geometry_stage_feature);

	gfx_render_stage_add_buffer(&geometry_stage, input->frame_data_buffer, GFX_BUFFER_ACCESS_graphics_r);
	gfx_render_stage_add_buffer(&geometry_stage, input->object_buffer, GFX_BUFFER_ACCESS_graphics_r);
	gfx_render_stage_add_buffer(&geometry_stage, input->indirect_buffer, GFX_BUFFER_ACCESS_indirect);

	for (int i = 0; i < GFX_GBUFFER_ATTACHMENT_max_enum; i++) {
		struct gfx_render_stage_attachment a = {0};
		
		gfx_render_stage_attachment_init_colour(&a,
							input->gbuffer->views[i],
							GFX_RENDER_SIZE_swapchain,
							true, v4(0.f, 0.f, 0.f, 1.f));
		
		gfx_render_stage_add_attachment(&geometry_stage, &a);
	}

	struct gfx_render_stage_attachment depth_attachment = {0};
	
	gfx_render_stage_attachment_init_depth(&depth_attachment,
					       input->gbuffer->views[i],
					       GFX_RENDER_SIZE_swapchain,
					       true, 1.f, 0);
	
	gfx_render_stage_add_attachment(&geometry_stage, &depth_attachment);
	
	gfx_render_graph_push(graph, &geometry_stage);

	// ---

	struct gfx_render_stage lighting_stage = {0};
	gfx_render_stage_init(&lighting_stage, GFX_RENDER_STAGE_graphics);

	gfx_render_stage_add_feature(&lighting_stage,
				     sizeof(struct stage_deferred_input), input,
				     lighting_stage_feature);

	for (int i = 0; i < GFX_GBUFFER_ATTACHMENT_max_enum; i++)
		gfx_render_stage_add_view(&lighting_stage, input->gbuffer->views[i], GFX_TEXTURE_ACCESS_graphics_r);

	gfx_render_stage_add_view(&lighting_stage,
				  gfx_device_texture_view_fetch_std(device, &input->probe->irradiance),
				  GFX_TEXTURE_ACCESS_graphics_r);

	gfx_render_stage_add_view(&lighting_stage,
				  gfx_device_texture_view_fetch_std(device, &input->probe->prefilter),
				  GFX_TEXTURE_ACCESS_graphics_r);

	struct gfx_render_stage_attachment lighting_attachment = {0};
	gfx_render_stage_attachment_init_colour(&lighting_attachment,
						input->lighting,
						GFX_RENDER_SIZE_swapchain,
						true, v4(0.f, 0.f, 0.f, 1.f));

	gfx_render_stage_add_attachment(&lighting_stage, &lighting_attachment);
	
	gfx_render_graph_push(graph, &lighting_stage);
}
