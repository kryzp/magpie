
internal VkPipelineLayout PipelineLayoutCreate(ShaderProgram *program)
{
	VkShaderStageFlags stage = ShaderProgramIsCompute(program)
		? VK_SHADER_STAGE_COMPUTE_BIT
		: VK_SHADER_STAGE_ALL_GRAPHICS;

	VkPushConstantRange push_constants = {0};
	push_constants.offset = 0;
	push_constants.size = program->push_constant_size;
	push_constants.stageFlags = stage;

	VkPipelineLayoutCreateInfo create_info = {0};
	create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	create_info.setLayoutCount = ArraySize(graphics_device->bindless.layouts);
	create_info.pSetLayouts = graphics_device->bindless.layouts;
	
	if (push_constants.size > 0) {
		create_info.pushConstantRangeCount = 1;
		create_info.pPushConstantRanges = &push_constants;
	} else {
		create_info.pushConstantRangeCount = 0;
		create_info.pPushConstantRanges = NULL;
	}

	VkPipelineLayout layout = VK_NULL_HANDLE;

	VK_CHECK(vkCreatePipelineLayout(graphics_device->device, &create_info, NULL,
					&layout),
		 "Failed to create pipeline layout.");

	return layout;
}

internal void PipelineLayoutDestroy(VkPipelineLayout layout)
{
	vkDestroyPipelineLayout(graphics_device->device, layout, NULL);
}

internal void PipelineDestroy(VkPipeline pipeline)
{
	vkDestroyPipeline(graphics_device->device, pipeline, NULL);
}

internal GraphicsPipelineDef GraphicsPipelineDefInitDefault(ShaderProgram *program)
{
	GraphicsPipelineDef def = {0};
	def.program = program;
	def.cull_mode = VK_CULL_MODE_BACK_BIT;
	def.front_face = VK_FRONT_FACE_CLOCKWISE;
	def.blend_state = BlendStateDefault();
	def.depth_stencil_state = DepthStencilStateDefault();
	def.has_depth_attachment = false;
	def.samples = VK_SAMPLE_COUNT_1_BIT;
	def.min_sample_shading_enabled = 1;
	def.min_sample_shading = 0.2f;
	def.view_mask = 0;

	return def;
}

internal VkPipeline GraphicsPipelineCreate(VkPipelineLayout layout,
					   GraphicsPipelineDef *definition)
{
	VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info = {0};
	vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input_state_create_info.vertexBindingDescriptionCount = 0;
	vertex_input_state_create_info.pVertexBindingDescriptions = NULL;
	vertex_input_state_create_info.vertexAttributeDescriptionCount = 0;
	vertex_input_state_create_info.pVertexAttributeDescriptions = NULL;

	VkPipelineInputAssemblyStateCreateInfo input_assembly_state_create_info = {0};
	input_assembly_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly_state_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	input_assembly_state_create_info.primitiveRestartEnable = VK_FALSE;

	VkPipelineViewportStateCreateInfo viewport_state_create_info = {0};
	viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_state_create_info.viewportCount = 1;
	viewport_state_create_info.pViewports = NULL; // Using dynamic viewport.
	viewport_state_create_info.scissorCount = 1;
	viewport_state_create_info.pScissors = NULL; // Using dynamic scissor.

	VkPipelineRasterizationStateCreateInfo rasterization_state_create_info = {0};
	rasterization_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterization_state_create_info.depthClampEnable = VK_FALSE;
	rasterization_state_create_info.rasterizerDiscardEnable = VK_FALSE;
	rasterization_state_create_info.polygonMode = VK_POLYGON_MODE_FILL;
	rasterization_state_create_info.lineWidth = 1.f;
	rasterization_state_create_info.cullMode = definition->cull_mode;
	rasterization_state_create_info.frontFace = definition->front_face;
	rasterization_state_create_info.depthBiasEnable = VK_FALSE;
	rasterization_state_create_info.depthBiasConstantFactor = 0.f;
	rasterization_state_create_info.depthBiasClamp = 0.f;
	rasterization_state_create_info.depthBiasSlopeFactor = 0.f;

	VkPipelineMultisampleStateCreateInfo multisample_state_create_info = {0};
	multisample_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisample_state_create_info.sampleShadingEnable = definition->min_sample_shading_enabled;
	multisample_state_create_info.minSampleShading = definition->min_sample_shading;
	multisample_state_create_info.rasterizationSamples = definition->samples;
	multisample_state_create_info.pSampleMask = NULL;
	multisample_state_create_info.alphaToCoverageEnable = VK_FALSE;
	multisample_state_create_info.alphaToOneEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState blend_states[MAX_COLOUR_ATTACHMENTS] = {0};

	VkPipelineColorBlendAttachmentState *blend_state = blend_states;

	for (i32 i = 0; i < definition->colour_attachment_count; i++, blend_state++) {

		blend_state->blendEnable = definition->blend_state.enabled;

		blend_state->srcColorBlendFactor = definition->blend_state.colour.src;
		blend_state->dstColorBlendFactor = definition->blend_state.colour.dst;
		blend_state->colorBlendOp = definition->blend_state.colour.op;

		blend_state->srcAlphaBlendFactor = definition->blend_state.alpha.src;
		blend_state->dstAlphaBlendFactor = definition->blend_state.alpha.dst;
		blend_state->alphaBlendOp = definition->blend_state.alpha.op;

		if (definition->blend_state.write_mask[0]) blend_state->colorWriteMask |= VK_COLOR_COMPONENT_R_BIT;
		if (definition->blend_state.write_mask[1]) blend_state->colorWriteMask |= VK_COLOR_COMPONENT_G_BIT;
		if (definition->blend_state.write_mask[2]) blend_state->colorWriteMask |= VK_COLOR_COMPONENT_B_BIT;
		if (definition->blend_state.write_mask[3]) blend_state->colorWriteMask |= VK_COLOR_COMPONENT_A_BIT;
	}

	VkPipelineColorBlendStateCreateInfo colour_blend_state_create_info = {0};
	colour_blend_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colour_blend_state_create_info.logicOpEnable = definition->blend_state.logic_op_enabled;
	colour_blend_state_create_info.logicOp = definition->blend_state.logic_op;
	colour_blend_state_create_info.attachmentCount = definition->colour_attachment_count;
	colour_blend_state_create_info.pAttachments = blend_states;
	colour_blend_state_create_info.blendConstants[0] = definition->blend_state.constants[0];
	colour_blend_state_create_info.blendConstants[1] = definition->blend_state.constants[1];
	colour_blend_state_create_info.blendConstants[2] = definition->blend_state.constants[2];
	colour_blend_state_create_info.blendConstants[3] = definition->blend_state.constants[3];

	VkPipelineDepthStencilStateCreateInfo depth_stencil_state_create_info = {0};
	depth_stencil_state_create_info.depthTestEnable = definition->depth_stencil_state.depth_test_enabled;
	depth_stencil_state_create_info.depthWriteEnable = definition->depth_stencil_state.depth_write_enabled;
	depth_stencil_state_create_info.depthCompareOp = definition->depth_stencil_state.depth_compare_op;
	depth_stencil_state_create_info.depthBoundsTestEnable = definition->depth_stencil_state.depth_bounds_test_enabled;
	depth_stencil_state_create_info.minDepthBounds = definition->depth_stencil_state.depth_bounds_min;
	depth_stencil_state_create_info.maxDepthBounds = definition->depth_stencil_state.depth_bounds_max;
	depth_stencil_state_create_info.stencilTestEnable = definition->depth_stencil_state.stencil_test_enabled;
	depth_stencil_state_create_info.front.failOp = definition->depth_stencil_state.stencil_front.fail_op;
	depth_stencil_state_create_info.front.passOp = definition->depth_stencil_state.stencil_front.pass_op;
	depth_stencil_state_create_info.front.depthFailOp = definition->depth_stencil_state.stencil_front.depth_fail_op;
	depth_stencil_state_create_info.front.compareOp = definition->depth_stencil_state.stencil_front.compare_op;
	depth_stencil_state_create_info.front.writeMask = definition->depth_stencil_state.stencil_front.write_mask;
	depth_stencil_state_create_info.front.reference = definition->depth_stencil_state.stencil_front.reference;
	depth_stencil_state_create_info.back.failOp = definition->depth_stencil_state.stencil_back.fail_op;
	depth_stencil_state_create_info.back.passOp = definition->depth_stencil_state.stencil_back.pass_op;
	depth_stencil_state_create_info.back.depthFailOp = definition->depth_stencil_state.stencil_back.depth_fail_op;
	depth_stencil_state_create_info.back.compareOp = definition->depth_stencil_state.stencil_back.compare_op;
	depth_stencil_state_create_info.back.writeMask = definition->depth_stencil_state.stencil_back.write_mask;
	depth_stencil_state_create_info.back.reference = definition->depth_stencil_state.stencil_back.reference;

	static VkDynamicState GRAPHICS_PIPELINE_DYNAMIC_STATES[] = {
		VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR,
		//VK_DYNAMIC_STATE_BLEND_CONSTANTS // TODO: Add dynamic blend constants.
	};

	VkPipelineDynamicStateCreateInfo dynamic_state_create_info = {0};
	dynamic_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamic_state_create_info.dynamicStateCount = ArraySize(GRAPHICS_PIPELINE_DYNAMIC_STATES);
	dynamic_state_create_info.pDynamicStates = GRAPHICS_PIPELINE_DYNAMIC_STATES;

	VkFormat depth_stencil_format = definition->has_depth_attachment
		? graphics_device->depth_format
		: VK_FORMAT_UNDEFINED;

	VkPipelineRenderingCreateInfo pipeline_rendering_create_info = {0};
	pipeline_rendering_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
	pipeline_rendering_create_info.viewMask = definition->view_mask;
	pipeline_rendering_create_info.colorAttachmentCount = definition->colour_attachment_count;
	pipeline_rendering_create_info.pColorAttachmentFormats = definition->colour_attachment_formats;
	pipeline_rendering_create_info.depthAttachmentFormat = depth_stencil_format;
	pipeline_rendering_create_info.stencilAttachmentFormat = depth_stencil_format;

	VkPipelineShaderStageCreateInfo shader_stages[2] = {0};

	for (i32 i = 0; i < definition->program->stage_count; i++) {
		VkPipelineShaderStageCreateInfo *shader_stage = shader_stages + i;

		shader_stage->sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shader_stage->stage = definition->program->stages[i].stage;
		shader_stage->module = definition->program->stages[i].module;
		shader_stage->pName = "main";
	}
	
	VkGraphicsPipelineCreateInfo graphics_pipeline_create_info = {0};
	graphics_pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	graphics_pipeline_create_info.stageCount = definition->program->stage_count;
	graphics_pipeline_create_info.pStages = shader_stages;
	graphics_pipeline_create_info.pVertexInputState = &vertex_input_state_create_info;
	graphics_pipeline_create_info.pInputAssemblyState = &input_assembly_state_create_info;
	graphics_pipeline_create_info.pViewportState = &viewport_state_create_info;
	graphics_pipeline_create_info.pRasterizationState = &rasterization_state_create_info;
	graphics_pipeline_create_info.pMultisampleState = &multisample_state_create_info;
	graphics_pipeline_create_info.pDepthStencilState = &depth_stencil_state_create_info;
	graphics_pipeline_create_info.pColorBlendState = &colour_blend_state_create_info;
	graphics_pipeline_create_info.pDynamicState = &dynamic_state_create_info;
	graphics_pipeline_create_info.layout = layout;
	graphics_pipeline_create_info.renderPass = VK_NULL_HANDLE;
	graphics_pipeline_create_info.subpass = 0;
	graphics_pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
	graphics_pipeline_create_info.basePipelineIndex = -1;
	graphics_pipeline_create_info.pNext = &pipeline_rendering_create_info;

	VkPipeline pipeline = VK_NULL_HANDLE;

	VK_CHECK(vkCreateGraphicsPipelines(graphics_device->device,
					   graphics_device->pipeline_process_cache, 1,
					   &graphics_pipeline_create_info, NULL,
					   &pipeline),
		 "Failed to create graphics pipeline.");

	return pipeline;
}

internal ComputePipelineDef ComputePipelineDefInit(ShaderProgram *program)
{
	ComputePipelineDef def = {0};
	def.program = program;

	return def;
}

internal VkPipeline ComputePipelineCreate(VkPipelineLayout layout,
					  ComputePipelineDef *definition)
{
	VkPipelineShaderStageCreateInfo shader_stage = {0};
	shader_stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shader_stage.stage = definition->program->stages[0].stage;
	shader_stage.module = definition->program->stages[0].module;
	shader_stage.pName = "main";

	VkComputePipelineCreateInfo compute_pipeline_create_info = {0};
	compute_pipeline_create_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	compute_pipeline_create_info.layout = layout;
	compute_pipeline_create_info.stage = shader_stage;

	VkPipeline pipeline = VK_NULL_HANDLE;

	VK_CHECK(vkCreateComputePipelines(graphics_device->device,
					  graphics_device->pipeline_process_cache, 1,
					  &compute_pipeline_create_info, NULL,
					  &pipeline),
		 "Failed to create compute pipeline.");

	return pipeline;
}

internal VkPipelineLayout FetchPipelineLayout(ShaderProgram *program)
{
	b32 is_compute = ShaderProgramIsCompute(program);

	u64 hash = 0;

	hash = HashBytesGenericCombine(hash, &is_compute,                  sizeof(b32));
	hash = HashBytesGenericCombine(hash, &program->push_constant_size, sizeof(u32));

	VkPipelineLayout *fetched_layout = HashTableFetchElement(&graphics_device->pipeline_layout_cache, hash);

	if (fetched_layout)
		return *fetched_layout;

	VkPipelineLayout layout = PipelineLayoutCreate(program);

	HashTableAddElement(&graphics_device->pipeline_layout_cache, hash, &layout);

	return layout;
}

internal PipelineState FetchGraphicsPipeline(GraphicsPipelineDef *definition)
{
	VkPipelineLayout layout = FetchPipelineLayout(definition->program);

	u64 hash = 0;

	hash = HashBytesGenericCombine(hash, definition->program,                     sizeof(ShaderProgram));
	hash = HashBytesGenericCombine(hash, &definition->cull_mode,                  sizeof(VkCullModeFlags));
	hash = HashBytesGenericCombine(hash, &definition->front_face,                 sizeof(VkFrontFace));
	hash = HashBytesGenericCombine(hash, &definition->blend_state,                sizeof(BlendState));
	hash = HashBytesGenericCombine(hash, &definition->depth_stencil_state,        sizeof(DepthStencilState));
	hash = HashBytesGenericCombine(hash, &definition->colour_attachment_count,    sizeof(u32));
	hash = HashBytesGenericCombine(hash, &definition->colour_attachment_formats,  sizeof(VkFormat) * MAX_COLOUR_ATTACHMENTS);
	hash = HashBytesGenericCombine(hash, &definition->has_depth_attachment,       sizeof(b32));
	hash = HashBytesGenericCombine(hash, &definition->samples,                    sizeof(VkSampleCountFlagBits));
	hash = HashBytesGenericCombine(hash, &definition->min_sample_shading_enabled, sizeof(b32));
	hash = HashBytesGenericCombine(hash, &definition->min_sample_shading,         sizeof(f32));
	hash = HashBytesGenericCombine(hash, &definition->view_mask,                  sizeof(u32));

	VkPipeline *fetched_pipeline = HashTableFetchElement(&graphics_device->pipeline_cache, hash);

	if (fetched_pipeline) {
		PipelineState st = {0};
		st.pipeline = *fetched_pipeline;
		st.layout = layout;
		st.bind_point = VK_PIPELINE_BIND_POINT_GRAPHICS;

		return st;
	}

	PipelineState st = {0};
	st.pipeline = GraphicsPipelineCreate(layout, definition);
	st.layout = layout;
	st.bind_point = VK_PIPELINE_BIND_POINT_GRAPHICS;

	HashTableAddElement(&graphics_device->pipeline_cache, hash, &st.pipeline);

	return st;
}

internal PipelineState FetchComputePipeline(ComputePipelineDef *definition)
{
	VkPipelineLayout layout = FetchPipelineLayout(definition->program);

	u64 hash = HashBytesGeneric(definition->program, sizeof(ShaderProgram));

	VkPipeline *fetched_pipeline = HashTableFetchElement(&graphics_device->pipeline_cache, hash);

	if (fetched_pipeline) {
		PipelineState st = {0};
		st.pipeline = *fetched_pipeline;
		st.layout = layout;
		st.bind_point = VK_PIPELINE_BIND_POINT_COMPUTE;

		return st;
	}

	PipelineState st = {0};
	st.pipeline = ComputePipelineCreate(layout, definition);
	st.layout = layout;
	st.bind_point = VK_PIPELINE_BIND_POINT_COMPUTE;
	
	HashTableAddElement(&graphics_device->pipeline_cache, hash, &st.pipeline);

	return st;
}
