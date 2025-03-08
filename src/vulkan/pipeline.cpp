#include "pipeline.h"

#include "backend.h"
#include "util.h"
#include "texture.h"
#include "shader.h"
#include "render_target.h"

using namespace llt;

Pipeline::Pipeline()
	: m_pipeline(VK_NULL_HANDLE)
	, m_bindPoint()
	, m_depthStencilCreateInfo()
	, m_sampleShadingEnabled()
	, m_minSampleShading()
	, m_cullMode()
	, m_currentVertexFormat()
	, m_computeShaderStageInfo()
	, m_boundShader(nullptr)
	, m_shaderStages()
{
	m_depthStencilCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	m_depthStencilCreateInfo.depthTestEnable = VK_TRUE;
	m_depthStencilCreateInfo.depthWriteEnable = VK_TRUE;
	m_depthStencilCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;
	m_depthStencilCreateInfo.depthBoundsTestEnable = VK_FALSE;
	m_depthStencilCreateInfo.minDepthBounds = 0.0f;
	m_depthStencilCreateInfo.maxDepthBounds = 1.0f;
	m_depthStencilCreateInfo.stencilTestEnable = VK_FALSE;
	m_depthStencilCreateInfo.front = {};
	m_depthStencilCreateInfo.back = {};
}

VkPipeline Pipeline::buildGraphicsPipeline(const RenderInfo &renderInfo)
{
	m_bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

	cauto &bindingDescriptions = m_currentVertexFormat.getBindingDescriptions();
	cauto &attributeDescriptions = m_currentVertexFormat.getAttributeDescriptions();

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
	viewportStateCreateInfo.pViewports = nullptr; // using dynamic viewports
	viewportStateCreateInfo.scissorCount = 1;
	viewportStateCreateInfo.pScissors = nullptr; // using dynamic scissor

	VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = {};
	rasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
	rasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
	rasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizationStateCreateInfo.lineWidth = 1.0f;
	rasterizationStateCreateInfo.cullMode = m_cullMode;
	rasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;
	rasterizationStateCreateInfo.depthBiasConstantFactor = 0.0f;
	rasterizationStateCreateInfo.depthBiasClamp = 0.0f;
	rasterizationStateCreateInfo.depthBiasSlopeFactor = 0.0f;

	VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = {};
	multisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleStateCreateInfo.sampleShadingEnable = m_sampleShadingEnabled ? VK_TRUE : VK_FALSE;
	multisampleStateCreateInfo.minSampleShading = m_minSampleShading;
	multisampleStateCreateInfo.rasterizationSamples = renderInfo.getMSAA();
	multisampleStateCreateInfo.pSampleMask = nullptr;
	multisampleStateCreateInfo.alphaToCoverageEnable = VK_FALSE;
	multisampleStateCreateInfo.alphaToOneEnable = VK_FALSE;

	cauto &blendAttachments = renderInfo.getColourBlendAttachmentStates();

	VkPipelineColorBlendStateCreateInfo colourBlendStateCreateInfo = {};
	colourBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colourBlendStateCreateInfo.logicOpEnable = renderInfo.isBlendStateLogicOpEnabled() ? VK_TRUE : VK_FALSE;
	colourBlendStateCreateInfo.logicOp = renderInfo.getBlendStateLogicOp();
	colourBlendStateCreateInfo.attachmentCount = blendAttachments.size();
	colourBlendStateCreateInfo.pAttachments = blendAttachments.data();
	colourBlendStateCreateInfo.blendConstants[0] = renderInfo.getBlendConstant(0);
	colourBlendStateCreateInfo.blendConstants[1] = renderInfo.getBlendConstant(1);
	colourBlendStateCreateInfo.blendConstants[2] = renderInfo.getBlendConstant(2);
	colourBlendStateCreateInfo.blendConstants[3] = renderInfo.getBlendConstant(3);

	uint64_t createdPipelineHash = 0;

	hash::combine(&createdPipelineHash, &colourBlendStateCreateInfo.logicOpEnable);
	hash::combine(&createdPipelineHash, &colourBlendStateCreateInfo.logicOp);
	hash::combine(&createdPipelineHash, &colourBlendStateCreateInfo.attachmentCount);
	hash::combine(&createdPipelineHash, &colourBlendStateCreateInfo.blendConstants[0]);
	hash::combine(&createdPipelineHash, &colourBlendStateCreateInfo.blendConstants[1]);
	hash::combine(&createdPipelineHash, &colourBlendStateCreateInfo.blendConstants[2]);
	hash::combine(&createdPipelineHash, &colourBlendStateCreateInfo.blendConstants[3]);

	hash::combine(&createdPipelineHash, &m_depthStencilCreateInfo);
	hash::combine(&createdPipelineHash, &rasterizationStateCreateInfo);
	hash::combine(&createdPipelineHash, &inputAssemblyStateCreateInfo);
	hash::combine(&createdPipelineHash, &multisampleStateCreateInfo);
	hash::combine(&createdPipelineHash, &viewportStateCreateInfo);

	for (auto &st : blendAttachments) {
		hash::combine(&createdPipelineHash, &st);
	}

	for (auto &attrib : attributeDescriptions) {
		hash::combine(&createdPipelineHash, &attrib);
	}

	for (auto &binding : bindingDescriptions) {
		hash::combine(&createdPipelineHash, &binding);
	}

	for (int i = 0; i < m_shaderStages.size(); i++) {
		hash::combine(&createdPipelineHash, &m_shaderStages[i]);
	}

	if (g_vulkanBackend->m_pipelineCache.contains(createdPipelineHash))
	{
		m_pipeline = g_vulkanBackend->m_pipelineCache[createdPipelineHash];

		LLT_LOG("Fetched new graphics pipeline from cache!");

		return m_pipeline;
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

	VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {};
	graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	graphicsPipelineCreateInfo.pStages = m_shaderStages.data();
	graphicsPipelineCreateInfo.stageCount = m_shaderStages.size();
	graphicsPipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;
	graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;
	graphicsPipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
	graphicsPipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
	graphicsPipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
	graphicsPipelineCreateInfo.pDepthStencilState = &m_depthStencilCreateInfo;
	graphicsPipelineCreateInfo.pColorBlendState = &colourBlendStateCreateInfo;
	graphicsPipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
	graphicsPipelineCreateInfo.layout = m_boundShader->getPipelineLayout();
	graphicsPipelineCreateInfo.renderPass = VK_NULL_HANDLE;
	graphicsPipelineCreateInfo.subpass = 0;
	graphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
	graphicsPipelineCreateInfo.basePipelineIndex = -1;
	graphicsPipelineCreateInfo.pNext = &pipelineRenderingCreateInfo;

	LLT_VK_CHECK(
		vkCreateGraphicsPipelines(g_vulkanBackend->m_device, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &m_pipeline),
		"Failed to create new graphics pipeline"
	);

	g_vulkanBackend->m_pipelineCache.insert(
		createdPipelineHash,
		m_pipeline
	);

	LLT_LOG("Created new graphics pipeline!");

	return m_pipeline;
}

VkPipeline Pipeline::buildComputePipeline()
{
	m_bindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;

	uint64_t createdPipelineHash = 0;

	hash::combine(&createdPipelineHash, &m_computeShaderStageInfo);

	if (g_vulkanBackend->m_pipelineCache.contains(createdPipelineHash))
	{
		m_pipeline = g_vulkanBackend->m_pipelineCache[createdPipelineHash];

		LLT_LOG("Fetched new compute pipeline from cache!");

		return m_pipeline;
	}

	VkComputePipelineCreateInfo computePipelineCreateInfo = {};
	computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	computePipelineCreateInfo.layout = m_boundShader->getPipelineLayout();
	computePipelineCreateInfo.stage = m_computeShaderStageInfo;

	LLT_VK_CHECK(
		vkCreateComputePipelines(g_vulkanBackend->m_device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &m_pipeline),
		"Failed to create new compute pipeline"
	);

	g_vulkanBackend->m_pipelineCache.insert(
		createdPipelineHash,
		m_pipeline
	);

	LLT_LOG("Created new compute pipeline!");

	return m_pipeline;
}

void Pipeline::bindShader(ShaderEffect *shader)
{
	if (!shader) {
		return;
	}

	m_boundShader = shader;

	for (auto &stage : shader->getStages())
	{
		switch (stage->getStage())
		{
			case VK_SHADER_STAGE_VERTEX_BIT:
				m_shaderStages[0] = stage->getShaderStageCreateInfo();
				break;

			case VK_SHADER_STAGE_FRAGMENT_BIT:
				m_shaderStages[1] = stage->getShaderStageCreateInfo();
				break;

			case VK_SHADER_STAGE_GEOMETRY_BIT:
				LLT_ERROR("Geometry shaders not yet supported!");

			case VK_SHADER_STAGE_MESH_BIT_EXT:
				LLT_ERROR("Mesh shaders not yet supported!");

			case VK_SHADER_STAGE_TASK_BIT_EXT:
				LLT_ERROR("Task shaders not yet supported!");

			case VK_SHADER_STAGE_COMPUTE_BIT:
				m_computeShaderStageInfo = stage->getShaderStageCreateInfo();
				break;

			default:
				LLT_ERROR("Unrecognised shader stage being bound to pipeline: %d", stage->getStage());
		}
	}
}

VkPipeline Pipeline::getPipeline()
{
	return m_pipeline;
}

VkPipelineBindPoint Pipeline::getBindPoint() const
{
	return m_bindPoint;
}

void Pipeline::setVertexFormat(const VertexFormat &descriptor)
{
	m_currentVertexFormat = descriptor;
}

void Pipeline::setSampleShading(bool enabled, float minSampleShading)
{
	m_sampleShadingEnabled = enabled;
	m_minSampleShading = minSampleShading;
}

void Pipeline::setCullMode(VkCullModeFlagBits cull)
{
	m_cullMode = cull;
}

void Pipeline::setDepthOp(VkCompareOp op)
{
	m_depthStencilCreateInfo.depthCompareOp = op;
}

void Pipeline::setDepthTest(bool enabled)
{
	m_depthStencilCreateInfo.depthTestEnable = enabled ? VK_TRUE : VK_FALSE;
}

void Pipeline::setDepthWrite(bool enabled)
{
	m_depthStencilCreateInfo.depthWriteEnable = enabled ? VK_TRUE : VK_FALSE;
}

void Pipeline::setDepthBounds(float min, float max)
{
	m_depthStencilCreateInfo.minDepthBounds = min;
	m_depthStencilCreateInfo.maxDepthBounds = max;
}

void Pipeline::setDepthStencilTest(bool enabled)
{
	m_depthStencilCreateInfo.stencilTestEnable = enabled ? VK_TRUE : VK_FALSE;
}

ShaderEffect *Pipeline::getShader()
{
	return m_boundShader;
}

const ShaderEffect *Pipeline::getShader() const
{
	return m_boundShader;
}
