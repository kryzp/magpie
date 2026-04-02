
internal GFX_GraphicsPipelineDef
GFX_GraphicsPipelineDefInit(GFX_ShaderKey program)
{
	GFX_GraphicsPipelineDef def = {0};

	def.program = program;

	def.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	def.cull_mode = VK_CULL_MODE_BACK_BIT;
	def.front_face = VK_FRONT_FACE_COUNTER_CLOCKWISE;

	def.colour_attachment_count = 0;
	def.has_depth_attachment = false;

	def.samples = VK_SAMPLE_COUNT_1_BIT;
	def.min_sample_shading_enabled = true;
	def.min_sample_shading = 0.2f;

	def.multi_view_mask = 0;
	
	return def;
}

internal GFX_ComputePipelineDef
GFX_ComputePipelineDefInit(GFX_ShaderKey program)
{
	GFX_ComputePipelineDef def = {0};

	def.program = program;

	return def;
}
