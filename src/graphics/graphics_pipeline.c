
internal G_GraphicsPipelineDef
G_GraphicsPipelineDefInit(G_ShaderKey program)
{
	G_GraphicsPipelineDef def = {0};

	def.program = program;

	def.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	def.cull_mode = VK_CULL_MODE_BACK_BIT;
	def.front_face = VK_FRONT_FACE_COUNTER_CLOCKWISE;

	def.blend_state = G_BlendStInit();
	def.depth_stencil_state = G_DepthStencilStInit();
	
	def.colour_attachment_count = 0;
	def.has_depth_attachment = false;

	def.samples = VK_SAMPLE_COUNT_1_BIT;
	def.min_sample_shading_enabled = true;
	def.min_sample_shading = 0.2f;

	def.multi_view_mask = 0;
	
	return def;
}

internal G_GraphicsPipelineDef
G_GraphicsPipelineDefFromInfo(G_ShaderKey program, const G_RenderInfo *info)
{
	G_GraphicsPipelineDef def = G_GraphicsPipelineDefInit(program);

	def.colour_attachment_count = info->colour_attachment_count;

	for (u32 i = 0; i < def.colour_attachment_count; i++)
		def.colour_attachment_formats[i] = info->colour_attachment_formats[i];
	
	def.has_depth_attachment = info->has_depth_attachment;

	def.samples = info->samples;
	
	def.multi_view_mask = info->view_mask;
	
	return def;
}

internal G_ComputePipelineDef
G_ComputePipelineDefInit(G_ShaderKey program)
{
	G_ComputePipelineDef def = {0};

	def.program = program;

	return def;
}
