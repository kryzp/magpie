#include "pipeline.h"

#include "backend.h"
#include "util.h"
#include "texture.h"
#include "shader.h"
#include "shader_buffer.h"
#include "render_target.h"
#include "render_pass.h"

using namespace llt;

Pipeline::Pipeline(VkShaderStageFlagBits stage, VkPipelineBindPoint bindPoint)
	: m_stage(stage)
	, m_descriptorSetLayout(VK_NULL_HANDLE)
	, m_pipeline(VK_NULL_HANDLE)
	, m_bindPoint(bindPoint)
{
}

Pipeline::~Pipeline()
{
}

VkPipeline Pipeline::getPipeline()
{
	return m_pipeline;
}

void Pipeline::bind(CommandBuffer &buffer)
{
	vkCmdBindPipeline(
		buffer.getBuffer(),
		m_bindPoint,
		m_pipeline
	);
}

void Pipeline::bindSet(CommandBuffer &buffer, const VkDescriptorSet &set)
{
	vkCmdBindDescriptorSets(
		buffer.getBuffer(),
		m_bindPoint,
		getPipelineLayout(),
		0,
		1, &set,
		0, nullptr
	);
}

void Pipeline::bindSet(CommandBuffer &buffer, const VkDescriptorSet &set, const Vector<uint32_t> &dynamicOffsets)
{
	vkCmdBindDescriptorSets(
		buffer.getBuffer(),
		m_bindPoint,
		getPipelineLayout(),
		0,
		1, &set,
		dynamicOffsets.size(),
		dynamicOffsets.data()
	);
}

VkPipelineLayout Pipeline::getPipelineLayout()
{
	uint64_t pipelineLayoutHash = 0;

	VkPushConstantRange pushConstants = {};
	pushConstants.offset = 0;
	pushConstants.size = g_vulkanBackend->m_pushConstants.size();
	pushConstants.stageFlags = m_stage;

	hash::combine(&pipelineLayoutHash, &pushConstants);

	hash::combine(&pipelineLayoutHash, &m_descriptorSetLayout);

	if (g_vulkanBackend->m_pipelineLayoutCache.contains(pipelineLayoutHash)) {
		return g_vulkanBackend->m_pipelineLayoutCache[pipelineLayoutHash];
	}

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.setLayoutCount = 1;
	pipelineLayoutCreateInfo.pSetLayouts = &m_descriptorSetLayout;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

	if (pushConstants.size > 0)
	{
		pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
		pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstants;
	}

	VkPipelineLayout pipelineLayout = {};

	LLT_VK_CHECK(
		vkCreatePipelineLayout(g_vulkanBackend->m_device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout),
		"Failed to create pipeline layout"
	);

	g_vulkanBackend->m_pipelineLayoutCache.insert(
		pipelineLayoutHash,
		pipelineLayout
	);

	LLT_LOG("Created new pipeline layout!");

	return pipelineLayout;
}

void Pipeline::setDescriptorSetLayout(const VkDescriptorSetLayout &layout)
{
	m_descriptorSetLayout = layout;
}

// --- graphics pipeline

GraphicsPipeline::GraphicsPipeline()
	: Pipeline(VK_SHADER_STAGE_ALL_GRAPHICS, VK_PIPELINE_BIND_POINT_GRAPHICS)
	, m_depthStencilCreateInfo()
	, m_samples(VK_SAMPLE_COUNT_1_BIT)
	, m_sampleShadingEnabled()
	, m_minSampleShading()
	, m_cullMode()
	, m_viewport()
	, m_scissor()
	, m_currentVertexFormat()
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

GraphicsPipeline::~GraphicsPipeline()
{
}

void GraphicsPipeline::render(CommandBuffer &buffer, const RenderPass &op)
{
	VkViewport viewport = getViewport();
	VkRect2D scissor = getScissor();

	vkCmdSetViewport(buffer.getBuffer(), 0, 1, &viewport);
	vkCmdSetScissor(buffer.getBuffer(), 0, 1, &scissor);

	VkPipelineLayout pipelineLayout = getPipelineLayout();

	if (g_vulkanBackend->m_pushConstants.size() > 0)
	{
		vkCmdPushConstants(
			buffer.getBuffer(),
			pipelineLayout,
			VK_SHADER_STAGE_ALL_GRAPHICS,
			0,
			g_vulkanBackend->m_pushConstants.size(),
			g_vulkanBackend->m_pushConstants.data()
		);
	}

	cauto &vertexBuffer = op.vertexBuffer->getBuffer();
	cauto &indexBuffer  = op.indexBuffer->getBuffer();

	VkBuffer pVertexBuffers[] = {
		vertexBuffer,
		VK_NULL_HANDLE
	};

	int nVertexBuffers = 1;

	if (op.instanceBuffer)
	{
		pVertexBuffers[1] = op.instanceBuffer->getBuffer();
		nVertexBuffers++;
	}

	cauto &bindings = m_currentVertexFormat.getBindingDescriptions();

	VkDeviceSize vertexBufferOffsets[] = { 0, 0 };

	vkCmdBindVertexBuffers(
		buffer.getBuffer(),
		bindings[0].binding,
		nVertexBuffers,
		pVertexBuffers,
		vertexBufferOffsets
	);

	VkDeviceSize indexBufferOffset = 0;

	vkCmdBindIndexBuffer(
		buffer.getBuffer(),
		indexBuffer,
		indexBufferOffset,
		VK_INDEX_TYPE_UINT16
	);

	if (op.indirectBuffer != nullptr)
	{
		// todo: start using vkCmdDrawIndirectCount
		vkCmdDrawIndexedIndirect(
			buffer.getBuffer(),
			op.indirectBuffer->getBuffer(),
			op.indirectOffset,
			op.indirectDrawCount,
			sizeof(VkDrawIndexedIndirectCommand)
		);
	}
	else
	{
		vkCmdDrawIndexed(
			buffer.getBuffer(),
			op.nIndices,
			op.instanceCount,
			0,
			0,
			op.firstInstance
		);
	}
}

void GraphicsPipeline::create(RenderInfo *renderInfo)
{
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

	if (m_viewport.width <= 0.0f || m_viewport.height <= 0.0f)
		resetViewport(*renderInfo);

	if (m_scissor.extent.width <= 0 || m_scissor.extent.height <= 0)
		resetScissor(*renderInfo);

	VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
	viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateCreateInfo.viewportCount = 1;
	viewportStateCreateInfo.pViewports = &m_viewport;
	viewportStateCreateInfo.scissorCount = 1;
	viewportStateCreateInfo.pScissors = &m_scissor;

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
	multisampleStateCreateInfo.rasterizationSamples = m_samples;
	multisampleStateCreateInfo.pSampleMask = nullptr;
	multisampleStateCreateInfo.alphaToCoverageEnable = VK_FALSE;
	multisampleStateCreateInfo.alphaToOneEnable = VK_FALSE;

	cauto &blendAttachments = renderInfo->getColourBlendAttachmentStates();

	VkPipelineColorBlendStateCreateInfo colourBlendStateCreateInfo = {};
	colourBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colourBlendStateCreateInfo.logicOpEnable = renderInfo->isBlendStateLogicOpEnabled() ? VK_TRUE : VK_FALSE;
	colourBlendStateCreateInfo.logicOp = renderInfo->getBlendStateLogicOp();
	colourBlendStateCreateInfo.attachmentCount = blendAttachments.size();
	colourBlendStateCreateInfo.pAttachments = blendAttachments.data();
	colourBlendStateCreateInfo.blendConstants[0] = renderInfo->getBlendConstant(0);
	colourBlendStateCreateInfo.blendConstants[1] = renderInfo->getBlendConstant(1);
	colourBlendStateCreateInfo.blendConstants[2] = renderInfo->getBlendConstant(2);
	colourBlendStateCreateInfo.blendConstants[3] = renderInfo->getBlendConstant(3);

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

	if (g_vulkanBackend->m_pipelineCache.contains(createdPipelineHash)) {
		m_pipeline = g_vulkanBackend->m_pipelineCache[createdPipelineHash];
		LLT_LOG("Fetched new graphics pipeline from cache!");
		return;
	}

	VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
	dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicStateCreateInfo.dynamicStateCount = LLT_ARRAY_LENGTH(vkutil::DYNAMIC_STATES);
	dynamicStateCreateInfo.pDynamicStates = vkutil::DYNAMIC_STATES;

	cauto &colourFormats = renderInfo->getColourAttachmentFormats();

	VkPipelineRenderingCreateInfo pipelineRenderingCreateInfo = {};
	pipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
	pipelineRenderingCreateInfo.colorAttachmentCount = colourFormats.size();
	pipelineRenderingCreateInfo.pColorAttachmentFormats = colourFormats.data();
	pipelineRenderingCreateInfo.depthAttachmentFormat = vkutil::findDepthFormat(g_vulkanBackend->m_physicalData.device);
	pipelineRenderingCreateInfo.stencilAttachmentFormat = vkutil::findDepthFormat(g_vulkanBackend->m_physicalData.device);

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
	graphicsPipelineCreateInfo.layout = getPipelineLayout();
	graphicsPipelineCreateInfo.renderPass = VK_NULL_HANDLE;
	graphicsPipelineCreateInfo.subpass = 0;
	graphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
	graphicsPipelineCreateInfo.basePipelineIndex = -1;
	graphicsPipelineCreateInfo.pNext = &pipelineRenderingCreateInfo;

	LLT_VK_CHECK(
		vkCreateGraphicsPipelines(g_vulkanBackend->m_device, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &m_pipeline),
		"Failed to create new graphics pipeline"
	);

	LLT_LOG("Created new graphics pipeline!");

	g_vulkanBackend->m_pipelineCache.insert(
		createdPipelineHash,
		m_pipeline
	);
}

void GraphicsPipeline::bindShader(const ShaderProgram *shader)
{
	if (!shader) {
		return;
	}

	switch (shader->type)
	{
		case VK_SHADER_STAGE_VERTEX_BIT:
			m_shaderStages[0] = shader->getShaderStageCreateInfo();
			break;

		case VK_SHADER_STAGE_FRAGMENT_BIT:
			m_shaderStages[1] = shader->getShaderStageCreateInfo();
			break;

		case VK_SHADER_STAGE_GEOMETRY_BIT:
			LLT_ERROR("Geometry shaders not yet implemented!!");

		default:
			LLT_ERROR("Unrecognised graphics shader type being bound: %d", shader->type);
	}
}

void GraphicsPipeline::setVertexFormat(const VertexFormat &descriptor)
{
	m_currentVertexFormat = descriptor;
}

void GraphicsPipeline::setMSAA(VkSampleCountFlagBits samples)
{
	m_samples = samples;
}

void GraphicsPipeline::setSampleShading(bool enabled, float minSampleShading)
{
	m_sampleShadingEnabled = enabled;
	m_minSampleShading = minSampleShading;
}

void GraphicsPipeline::setCullMode(VkCullModeFlagBits cull)
{
	m_cullMode = cull;
}

void GraphicsPipeline::setViewport(const RectF &rect)
{
	m_viewport = (VkViewport) { rect.x, rect.y, rect.w, rect.h };
}

void GraphicsPipeline::setScissor(const RectI &rect)
{
	m_scissor = (VkRect2D) { { rect.x, rect.y }, { (uint32_t)rect.w, (uint32_t)rect.h } };
}

VkViewport GraphicsPipeline::getViewport() const
{
	return m_viewport;
}

VkRect2D GraphicsPipeline::getScissor() const
{
	return m_scissor;
}

void GraphicsPipeline::resetViewport(const RenderInfo &info)
{
	m_viewport.x = 0.0f;
	m_viewport.y = (float)info.getHeight();
	m_viewport.width = (float)info.getWidth();
	m_viewport.height = -(float)info.getHeight();
	m_viewport.minDepth = 0.0f;
	m_viewport.maxDepth = 1.0f;
}

void GraphicsPipeline::resetScissor(const RenderInfo &info)
{
	m_scissor.offset = { 0, 0 };
	m_scissor.extent = { info.getWidth(), info.getHeight() };
}

void GraphicsPipeline::setDepthOp(VkCompareOp op)
{
	m_depthStencilCreateInfo.depthCompareOp = op;
}

void GraphicsPipeline::setDepthTest(bool enabled)
{
	m_depthStencilCreateInfo.depthTestEnable = enabled ? VK_TRUE : VK_FALSE;
}

void GraphicsPipeline::setDepthWrite(bool enabled)
{
	m_depthStencilCreateInfo.depthWriteEnable = enabled ? VK_TRUE : VK_FALSE;
}

void GraphicsPipeline::setDepthBounds(float min, float max)
{
	m_depthStencilCreateInfo.minDepthBounds = min;
	m_depthStencilCreateInfo.maxDepthBounds = max;
}

void GraphicsPipeline::setDepthStencilTest(bool enabled)
{
	m_depthStencilCreateInfo.stencilTestEnable = enabled ? VK_TRUE : VK_FALSE;
}

// --- compute pipeline

ComputePipeline::ComputePipeline()
	: Pipeline(VK_SHADER_STAGE_COMPUTE_BIT, VK_PIPELINE_BIND_POINT_COMPUTE)
	, m_computeShaderStageInfo()
{
}

ComputePipeline::~ComputePipeline()
{
}

void ComputePipeline::dispatch(CommandBuffer &buffer, int gcX, int gcY, int gcZ)
{
	//p_boundTextures.pushPipelineBarriers(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

	VkPipelineLayout pipelineLayout = getPipelineLayout();

	if (g_vulkanBackend->m_pushConstants.size() > 0)
	{
		vkCmdPushConstants(
			buffer.getBuffer(),
			pipelineLayout,
			VK_SHADER_STAGE_COMPUTE_BIT,
			0,
			g_vulkanBackend->m_pushConstants.size(),
			g_vulkanBackend->m_pushConstants.data()
		);
	}

	vkCmdDispatch(
		buffer.getBuffer(),
		gcX, gcY, gcZ
	);

	//p_boundTextures.popPipelineBarriers();
}

void ComputePipeline::create(RenderInfo *renderInfo)
{
	VkPipelineLayout layout = getPipelineLayout();

	uint64_t createdPipelineHash = 0;

	hash::combine(&createdPipelineHash, &m_computeShaderStageInfo);
	hash::combine(&createdPipelineHash, &layout);

	if (g_vulkanBackend->m_pipelineCache.contains(createdPipelineHash)) {
		m_pipeline = g_vulkanBackend->m_pipelineCache[createdPipelineHash];
		LLT_LOG("Fetched new compute pipeline from cache!");
		return;
	}

	VkComputePipelineCreateInfo computePipelineCreateInfo = {};
	computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	computePipelineCreateInfo.layout = layout;
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
}

void ComputePipeline::bindShader(const ShaderProgram *shader)
{
	if (!shader) {
		return;
	}

	switch (shader->type)
	{
		case VK_SHADER_STAGE_COMPUTE_BIT:
			m_computeShaderStageInfo = shader->getShaderStageCreateInfo();
			break;

		default:
			LLT_ERROR("Unrecognised compute shader type being bound: %d", shader->type);
			break;
	}
}
