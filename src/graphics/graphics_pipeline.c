
internal GFX_GraphicsPipelineDef
GFX_GraphicsPipelineDefInit(GFX_ShaderKey program)
{
	GFX_GraphicsPipelineDef def = {0};

	def.program = program;

	def.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	def.cull_mode = VK_CULL_MODE_BACK_BIT;
	def.front_face = VK_FRONT_FACE_COUNTER_CLOCKWISE;

	def.blend_state = GFX_BlendStInit();
	def.depth_stencil_state = GFX_DepthStencilStInit();
	
	def.colour_attachment_count = 0;
	def.has_depth_attachment = false;

	def.samples = VK_SAMPLE_COUNT_1_BIT;
	def.min_sample_shading_enabled = true;
	def.min_sample_shading = 0.2f;

	def.multi_view_mask = 0;
	
	return def;
}

internal GFX_GraphicsPipelineDef
GFX_GraphicsPipelineDefFromInfo(GFX_ShaderKey program, const GFX_RenderInfo *info)
{
	GFX_GraphicsPipelineDef def = GFX_GraphicsPipelineDefInit(program);

	def.colour_attachment_count = info->colour_attachment_count;

	for (u32 i = 0; i < def.colour_attachment_count; i++)
		def.colour_attachment_formats[i] = info->colour_attachment_formats[i];
	
	def.has_depth_attachment = info->has_depth_attachment;

	def.samples = info->samples;
	
	def.multi_view_mask = info->view_mask;
	
	return def;
}

internal GFX_ComputePipelineDef
GFX_ComputePipelineDefInit(GFX_ShaderKey program)
{
	GFX_ComputePipelineDef def = {0};

	def.program = program;

	return def;
}
