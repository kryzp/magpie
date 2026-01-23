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
