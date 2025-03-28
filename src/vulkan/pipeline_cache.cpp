#include "pipeline_cache.h"

#include "core.h"
#include "util.h"
#include "texture.h"
#include "shader.h"
#include "render_target.h"

using namespace llt;

PipelineCache::PipelineCache()
	: m_pipelines()
	, m_layouts()
{
}

PipelineCache::~PipelineCache()
{
	dispose();
}

void PipelineCache::init()
{
}

void PipelineCache::dispose()
{
	for (auto &[id, cache] : m_pipelines) {
		vkDestroyPipeline(g_vkCore->m_device, cache, nullptr);
	}

	m_pipelines.clear();

	for (auto &[id, cache] : m_layouts) {
		vkDestroyPipelineLayout(g_vkCore->m_device, cache, nullptr);
	}

	m_layouts.clear();
}

PipelineData PipelineCache::fetchGraphicsPipeline(const GraphicsPipelineDefinition &definition, const RenderInfo &renderInfo)
{
	cauto &bindingDescriptions = definition.getVertexFormat().getBindingDescriptions();
	cauto &attributeDescriptions = definition.getVertexFormat().getAttributeDescriptions();

	VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {};
	vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputStateCreateInfo.vertexBindingDescriptionCount = bindingDescriptions.size();
	vertexInputStateCreateInfo.pVertexBindingDescriptions = bindingDescriptions.data();
	vertexInputStateCreateInfo.vertexAttributeDescriptionCount = attributeDescriptions.size();
	vertexInputStateCreateInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo = {};
	inputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;

	VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
	viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateCreateInfo.viewportCount = 1;
	viewportStateCreateInfo.pViewports = nullptr; // using dynamic viewport
	viewportStateCreateInfo.scissorCount = 1;
	viewportStateCreateInfo.pScissors = nullptr; // using dynamic scissor

	VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = {};
	rasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
	rasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
	rasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizationStateCreateInfo.lineWidth = 1.0f;
	rasterizationStateCreateInfo.cullMode = definition.getCullMode();
	rasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;
	rasterizationStateCreateInfo.depthBiasConstantFactor = 0.0f;
	rasterizationStateCreateInfo.depthBiasClamp = 0.0f;
	rasterizationStateCreateInfo.depthBiasSlopeFactor = 0.0f;

	VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = {};
	multisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleStateCreateInfo.sampleShadingEnable = definition.isSampleShadingEnabled() ? VK_TRUE : VK_FALSE;
	multisampleStateCreateInfo.minSampleShading = definition.getMinSampleShading();
	multisampleStateCreateInfo.rasterizationSamples = renderInfo.getMSAA();
	multisampleStateCreateInfo.pSampleMask = nullptr;
	multisampleStateCreateInfo.alphaToCoverageEnable = VK_FALSE;
	multisampleStateCreateInfo.alphaToOneEnable = VK_FALSE;

	Vector<VkPipelineColorBlendAttachmentState> blendStates;

	for (int i = 0; i < renderInfo.getColourAttachmentCount(); i++) {
		blendStates.pushBack(definition.getColourBlendState());
	}

	VkPipelineColorBlendStateCreateInfo colourBlendStateCreateInfo = {};
	colourBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colourBlendStateCreateInfo.logicOpEnable = definition.isBlendStateLogicOpEnabled() ? VK_TRUE : VK_FALSE;
	colourBlendStateCreateInfo.logicOp = definition.getBlendStateLogicOp();
	colourBlendStateCreateInfo.attachmentCount = blendStates.size();
	colourBlendStateCreateInfo.pAttachments = blendStates.data();
	colourBlendStateCreateInfo.blendConstants[0] = definition.getBlendConstant(0);
	colourBlendStateCreateInfo.blendConstants[1] = definition.getBlendConstant(1);
	colourBlendStateCreateInfo.blendConstants[2] = definition.getBlendConstant(2);
	colourBlendStateCreateInfo.blendConstants[3] = definition.getBlendConstant(3);

	uint64_t createdPipelineHash = 0;

	hash::combine(&createdPipelineHash, &colourBlendStateCreateInfo.logicOpEnable);
	hash::combine(&createdPipelineHash, &colourBlendStateCreateInfo.logicOp);
	hash::combine(&createdPipelineHash, &colourBlendStateCreateInfo.attachmentCount);
	hash::combine(&createdPipelineHash, &colourBlendStateCreateInfo.blendConstants[0]);
	hash::combine(&createdPipelineHash, &colourBlendStateCreateInfo.blendConstants[1]);
	hash::combine(&createdPipelineHash, &colourBlendStateCreateInfo.blendConstants[2]);
	hash::combine(&createdPipelineHash, &colourBlendStateCreateInfo.blendConstants[3]);

	hash::combine(&createdPipelineHash, &definition.getColourBlendState());
	hash::combine(&createdPipelineHash, &definition.getDepthStencilState());
	hash::combine(&createdPipelineHash, &rasterizationStateCreateInfo);
	hash::combine(&createdPipelineHash, &inputAssemblyStateCreateInfo);
	hash::combine(&createdPipelineHash, &multisampleStateCreateInfo);
	hash::combine(&createdPipelineHash, &viewportStateCreateInfo);

	for (auto &attrib : attributeDescriptions) {
		hash::combine(&createdPipelineHash, &attrib);
	}

	for (auto &binding : bindingDescriptions) {
		hash::combine(&createdPipelineHash, &binding);
	}

	for (int i = 0; i < definition.getShaderStages().size(); i++) {
		hash::combine(&createdPipelineHash, &definition.getShaderStage(i));
	}

	if (m_pipelines.contains(createdPipelineHash))
	{
		VkPipelineLayout layout = fetchPipelineLayout(definition.getShader());

		PipelineData data = {};
		data.pipeline = m_pipelines[createdPipelineHash];
		data.layout = layout;

		return data;
	}

	VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
	dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicStateCreateInfo.dynamicStateCount = LLT_ARRAY_LENGTH(vkutil::DYNAMIC_STATES);
	dynamicStateCreateInfo.pDynamicStates = vkutil::DYNAMIC_STATES;

	cauto &colourFormats = renderInfo.getColourAttachmentFormats();
	VkFormat depthStencilFormat = renderInfo.getDepthAttachmentFormat();

	VkPipelineRenderingCreateInfo pipelineRenderingCreateInfo = {};
	pipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
	pipelineRenderingCreateInfo.colorAttachmentCount = colourFormats.size();
	pipelineRenderingCreateInfo.pColorAttachmentFormats = colourFormats.data();
	pipelineRenderingCreateInfo.depthAttachmentFormat = depthStencilFormat;
	pipelineRenderingCreateInfo.stencilAttachmentFormat = depthStencilFormat;

	VkPipelineLayout layout = fetchPipelineLayout(definition.getShader());

	VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {};
	graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	graphicsPipelineCreateInfo.pStages = definition.getShaderStages().data();
	graphicsPipelineCreateInfo.stageCount = definition.getShaderStages().size();
	graphicsPipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;
	graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;
	graphicsPipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
	graphicsPipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
	graphicsPipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
	graphicsPipelineCreateInfo.pDepthStencilState = &definition.getDepthStencilState();
	graphicsPipelineCreateInfo.pColorBlendState = &colourBlendStateCreateInfo;
	graphicsPipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
	graphicsPipelineCreateInfo.layout = layout;
	graphicsPipelineCreateInfo.renderPass = VK_NULL_HANDLE;
	graphicsPipelineCreateInfo.subpass = 0;
	graphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
	graphicsPipelineCreateInfo.basePipelineIndex = -1;
	graphicsPipelineCreateInfo.pNext = &pipelineRenderingCreateInfo;

	VkPipeline pipeline = VK_NULL_HANDLE;

	LLT_VK_CHECK(
		vkCreateGraphicsPipelines(g_vkCore->m_device, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &pipeline),
		"Failed to create new graphics pipeline"
	);

	m_pipelines.insert(
		createdPipelineHash,
		pipeline
	);

	LLT_LOG("Created new graphics pipeline!");

	PipelineData data = {};
	data.pipeline = pipeline;
	data.layout = layout;

	return data;
}

PipelineData PipelineCache::fetchComputePipeline(const ComputePipelineDefinition &definition)
{
	uint64_t createdPipelineHash = 0;

	hash::combine(&createdPipelineHash, &definition.getStage());

	if (m_pipelines.contains(createdPipelineHash))
	{
		VkPipelineLayout layout = fetchPipelineLayout(definition.getShader());

		PipelineData data = {};
		data.pipeline = m_pipelines[createdPipelineHash];
		data.layout = layout;

		return data;
	}

	VkPipelineLayout layout = fetchPipelineLayout(definition.getShader());

	VkComputePipelineCreateInfo computePipelineCreateInfo = {};
	computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	computePipelineCreateInfo.layout = layout;
	computePipelineCreateInfo.stage = definition.getStage();

	VkPipeline pipeline = VK_NULL_HANDLE;

	LLT_VK_CHECK(
		vkCreateComputePipelines(g_vkCore->m_device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &pipeline),
		"Failed to create new compute pipeline"
	);

	m_pipelines.insert(
		createdPipelineHash,
		pipeline
	);

	LLT_LOG("Created new compute pipeline!");

	PipelineData data = {};
	data.pipeline = pipeline;
	data.layout = layout;

	return data;
}

VkPipelineLayout PipelineCache::fetchPipelineLayout(const ShaderEffect *shader)
{
	VkShaderStageFlagBits shaderStage = shader->getStage(0)->getStage() == VK_SHADER_STAGE_COMPUTE_BIT ? VK_SHADER_STAGE_COMPUTE_BIT : VK_SHADER_STAGE_ALL_GRAPHICS;

	cauto &setLayouts = shader->getDescriptorSetLayouts();

	uint64_t pipelineLayoutHash = 0;

	VkPushConstantRange pushConstants = {};
	pushConstants.offset = 0;
	pushConstants.size = shader->getPushConstantsSize();
	pushConstants.stageFlags = shaderStage;

	hash::combine(&pipelineLayoutHash, &pushConstants);

	for (auto &layout : setLayouts)
	{
		hash::combine(&pipelineLayoutHash, &layout);
	}

	if (m_layouts.contains(pipelineLayoutHash))
	{
		return m_layouts[pipelineLayoutHash];
	}

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.setLayoutCount = setLayouts.size();
	pipelineLayoutCreateInfo.pSetLayouts = setLayouts.data();
	pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

	if (pushConstants.size > 0)
	{
		pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
		pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstants;
	}

	VkPipelineLayout layout = VK_NULL_HANDLE;

	LLT_VK_CHECK(
		vkCreatePipelineLayout(g_vkCore->m_device, &pipelineLayoutCreateInfo, nullptr, &layout),
		"Failed to create pipeline layout"
	);

	m_layouts.insert(
		pipelineLayoutHash,
		layout
	);

	LLT_LOG("Created new pipeline layout!");

	return layout;
}
