
struct geometry_pass_context {
	GBuffer *gbuffer;
	MeshPass *mesh_pass;
	GPUBuffer *frame_data_buffer;
	GPUBuffer *object_buffer;
	GPUBuffer *instance_buffer;
	GPUBuffer *indirect_buffer;
};

internal void RenderPassGeometry(RenderState *rs, RenderInfo *render_info, void *context)
{
	CommandBuffer *cmd = &rs->cmd;
	
	struct geometry_pass_context *pass_context = (struct geometry_pass_context *)context;

	GraphicsPipelineDef pipeline_def = GraphicsPipelineDefInitDefault(&shaders->model_program);
	pipeline_def.colour_attachment_count = GBufferAttachment_MaxEnum;
	for (i32 i = 0; i < GBufferAttachment_MaxEnum; i++)
		pipeline_def.colour_attachment_formats[i] = pass_context->gbuffer->attachments[i].format;
	pipeline_def.has_depth_attachment = true;

	PipelineState pipeline_st = FetchGraphicsPipeline(&pipeline_def);

	CmdBindBindless(cmd, pipeline_st.bind_point, pipeline_st.layout);
	CmdBindPipeline(cmd, pipeline_st.bind_point, pipeline_st.pipeline);

	for (MultiBatch *multi_batch = pass_context->mesh_pass->multi_batches; multi_batch; multi_batch = multi_batch->next) {

		IndirectBatch *batch = pass_context->mesh_pass->batches;
		for (u32 k = 0; k < multi_batch->first; k++, batch = batch->next);
		
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
		args.sampler = core->linear_sampler.resource_id;

		CmdPushConstants(cmd, pipeline_st.layout, VK_SHADER_STAGE_ALL_GRAPHICS, sizeof(args), &args);

		MeshBindIndices(rs->meshes[batch->mesh_id].original, cmd);
	
		CmdDrawIndexedIndirect(cmd, pass_context->indirect_buffer,
				       sizeof(GPU_Indirect) * multi_batch->first,
				       multi_batch->count,
				       sizeof(GPU_Indirect));
	}
}

struct lighting_pass_context {
	GBuffer *gbuffer;
	ImageView *target;
	EnvironmentProbe *probe;
	GPUBuffer *frame_data_buffer;
	GPUBuffer *light_buffer;
};

internal void RenderPassLighting(RenderState *rs, RenderInfo *render_info, void *context)
{
	CommandBuffer *cmd = &rs->cmd;

	struct lighting_pass_context *pass_context = (struct lighting_pass_context *)context;
	GBuffer *gbuffer = pass_context->gbuffer;
	EnvironmentProbe *probe = pass_context->probe;

	PipelineState pipeline_st = {0};
	
	// Ambient Lighting.
	GraphicsPipelineDef ambient_pipeline_def = GraphicsPipelineDefInitDefault(&shaders->ambient_lighting_program);
	ambient_pipeline_def.depth_stencil_state.depth_test_enabled = false;
	ambient_pipeline_def.depth_stencil_state.depth_write_enabled = false;
	ambient_pipeline_def.colour_attachment_count = 1;
	ambient_pipeline_def.colour_attachment_formats[0] = pass_context->target->image->format;
	
	pipeline_st = FetchGraphicsPipeline(&ambient_pipeline_def);

	CmdBindBindless(cmd, pipeline_st.bind_point, pipeline_st.layout);
	CmdBindPipeline(cmd, pipeline_st.bind_point, pipeline_st.pipeline);

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

	pc_ambient.position = FetchStandardImageView(gbuffer->attachments + GBufferAttachment_Position)->resource_id;
	pc_ambient.albedo   = FetchStandardImageView(gbuffer->attachments + GBufferAttachment_Albedo)->resource_id;
	pc_ambient.normal   = FetchStandardImageView(gbuffer->attachments + GBufferAttachment_Normal)->resource_id;
	pc_ambient.material = FetchStandardImageView(gbuffer->attachments + GBufferAttachment_MetallicRoughness)->resource_id;
	pc_ambient.emissive = FetchStandardImageView(gbuffer->attachments + GBufferAttachment_Emissive)->resource_id;

	pc_ambient.irradiance_map = FetchStandardImageView(&probe->irradiance)->resource_id;
	pc_ambient.prefilter_map = FetchStandardImageView(&probe->prefilter)->resource_id;
	pc_ambient.brdf_lut = FetchStandardImageView(&core->brdf_lut_image)->resource_id;

	pc_ambient.linear_sampler = core->linear_sampler.resource_id;

	CmdPushConstants(cmd, pipeline_st.layout, VK_SHADER_STAGE_ALL_GRAPHICS, sizeof(pc_ambient), &pc_ambient);
	CmdDrawVerticesN(cmd, 3);

	// Direct lighting.
	GraphicsPipelineDef direct_pipeline_def = GraphicsPipelineDefInitDefault(&shaders->direct_lighting_point_program);
	direct_pipeline_def.depth_stencil_state.depth_test_enabled = false;
	direct_pipeline_def.depth_stencil_state.depth_write_enabled = false;
	direct_pipeline_def.cull_mode = VK_CULL_MODE_FRONT_BIT;
	direct_pipeline_def.colour_attachment_count = 1;
	direct_pipeline_def.colour_attachment_formats[0] = pass_context->target->image->format;
	direct_pipeline_def.blend_state.enabled = true;
	direct_pipeline_def.blend_state.colour.op = VK_BLEND_OP_ADD;
	direct_pipeline_def.blend_state.colour.dst = VK_BLEND_FACTOR_ONE;
	direct_pipeline_def.blend_state.colour.src = VK_BLEND_FACTOR_ONE;

	pipeline_st = FetchGraphicsPipeline(&direct_pipeline_def);

	CmdBindBindless(cmd, pipeline_st.bind_point, pipeline_st.layout);
	CmdBindPipeline(cmd, pipeline_st.bind_point, pipeline_st.pipeline);

	MeshBindIndices(&core->light_sphere_mesh, cmd);
	
	for (u32 i = 0; i < rs->light_count; i++) {
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

		pc_direct.frame_data_buffer = pass_context->frame_data_buffer->device_address;
		pc_direct.light_buffer      = pass_context->light_buffer->device_address;
		pc_direct.vertex_buffer     = core->light_sphere_mesh.vertex_buffer.device_address;
				
		pc_direct.position = FetchStandardImageView(gbuffer->attachments + GBufferAttachment_Position)->resource_id;
		pc_direct.albedo   = FetchStandardImageView(gbuffer->attachments + GBufferAttachment_Albedo)->resource_id;
		pc_direct.normal   = FetchStandardImageView(gbuffer->attachments + GBufferAttachment_Normal)->resource_id;
		pc_direct.material = FetchStandardImageView(gbuffer->attachments + GBufferAttachment_MetallicRoughness)->resource_id;
		pc_direct.emissive = FetchStandardImageView(gbuffer->attachments + GBufferAttachment_Emissive)->resource_id;
				
		pc_direct.linear_sampler = core->linear_sampler.resource_id;

		CmdPushConstants(cmd, pipeline_st.layout, VK_SHADER_STAGE_ALL_GRAPHICS, sizeof(pc_direct), &pc_direct);

		MeshDrawIndexedID(&core->light_sphere_mesh, cmd, i);
	}
}

// ---

internal void GBufferInit(GBuffer *gbuffer)
{
	for (i32 i = 0; i < GBufferAttachment_MaxEnum; i++) {
		gbuffer->attachments[i] = ImageAlloc2D(graphics_device->swapchain.width, graphics_device->swapchain.height,
						       VK_FORMAT_R32G32B32A32_SFLOAT, 1);
		
		gbuffer->views[i] = FetchStandardImageView(&gbuffer->attachments[i]);
	}
	
	gbuffer->depth = ImageAlloc2D(graphics_device->swapchain.width, graphics_device->swapchain.height,
				      graphics_device->depth_format, 1);

	gbuffer->depth_view = FetchStandardImageView(&gbuffer->depth);
}

internal void GBufferDestroy(GBuffer *gbuffer)
{
	for (i32 i = 0; i < GBufferAttachment_MaxEnum; i++)
		ImageDestroy(gbuffer->attachments + i);

	ImageDestroy(&gbuffer->depth);
}

struct deferred_renderer_input {
	Scene *scene;
	Camera *camera;
	EnvironmentProbe *probe;
	MeshPass *mesh_pass;
	GPUBuffer *frame_data_buffer;
	GPUBuffer *object_buffer;
	GPUBuffer *instance_buffer;
	GPUBuffer *indirect_buffer;
	GPUBuffer *light_buffer;
	GBuffer *gbuffer;
	ImageView *lighting;
};

internal void DeferredRenderFrame(RenderGraph *graph,
				  struct deferred_renderer_input *input)
{
	// --- GEOMETRY PASS

	struct geometry_pass_context geometry_context = {
		.gbuffer = input->gbuffer,
		.mesh_pass = input->mesh_pass,
		.frame_data_buffer = input->frame_data_buffer,
		.object_buffer = input->object_buffer,
		.instance_buffer = input->instance_buffer,
		.indirect_buffer = input->indirect_buffer
	};
	
	RenderPass gbuffer_render_pass = {0};
	gbuffer_render_pass.type = RenderPassType_Graphics;
	gbuffer_render_pass.graphics.Record = RenderPassGeometry;
	gbuffer_render_pass.graphics.buffer_count = 3;
	gbuffer_render_pass.graphics.buffers[0] = input->frame_data_buffer;
	gbuffer_render_pass.graphics.buffers[1] = input->object_buffer;
	gbuffer_render_pass.graphics.buffers[2] = input->indirect_buffer;
	gbuffer_render_pass.graphics.attachment_count = GBufferAttachment_MaxEnum + 1;

	for (i32 i = 0; i < GBufferAttachment_MaxEnum; i++)
		gbuffer_render_pass.graphics.attachments[i] = RenderingAttachmentInitColour(VK_ATTACHMENT_LOAD_OP_CLEAR,
											    input->gbuffer->views[i],
											    NULL, v4(0.f, 0.f, 0.f, 1.f));

	gbuffer_render_pass.graphics.attachments[GBufferAttachment_MaxEnum] = RenderingAttachmentInitDepth(VK_ATTACHMENT_LOAD_OP_CLEAR,
													   input->gbuffer->depth_view,
													   NULL, 1.f, 0);

	MemoryCopy(gbuffer_render_pass.context, &geometry_context, sizeof(geometry_context));
	
	RenderGraphPush(graph, &gbuffer_render_pass);

	// --- LIGHTING PASS

	struct lighting_pass_context lighting_context = {
		.gbuffer = input->gbuffer,
		.target = input->lighting,
		.probe = input->probe,
		.frame_data_buffer = input->frame_data_buffer,
		.light_buffer = input->light_buffer
	};
	
	RenderPass lighting_render_pass = {0};
	lighting_render_pass.type = RenderPassType_Graphics;
	lighting_render_pass.graphics.Record = RenderPassLighting;
	lighting_render_pass.graphics.buffer_count = 2;
	lighting_render_pass.graphics.buffers[0] = input->frame_data_buffer;
	lighting_render_pass.graphics.buffers[1] = input->light_buffer;
	lighting_render_pass.graphics.view_count = GBufferAttachment_MaxEnum + 2;

	for (i32 i = 0; i < GBufferAttachment_MaxEnum; i++)
		lighting_render_pass.graphics.views[i] = FetchStandardImageView(input->gbuffer->attachments + i);

	lighting_render_pass.graphics.views[GBufferAttachment_MaxEnum + 0] = FetchStandardImageView(&input->probe->irradiance);
	lighting_render_pass.graphics.views[GBufferAttachment_MaxEnum + 1] = FetchStandardImageView(&input->probe->prefilter);

	lighting_render_pass.graphics.attachment_count = 1;
	lighting_render_pass.graphics.attachments[0] = RenderingAttachmentInitColour(VK_ATTACHMENT_LOAD_OP_CLEAR,
										     input->lighting,
										     NULL, v4(0.f, 0.f, 0.f, 1.f));

	MemoryCopy(lighting_render_pass.context, &lighting_context, sizeof(lighting_context));

	RenderGraphPush(graph, &lighting_render_pass);
}
