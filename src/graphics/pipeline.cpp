#include "pipeline.h"

#include "backend.h"
#include "util.h"
#include "texture.h"
#include "shader.h"
#include "shader_buffer.h"
#include "render_target.h"
#include "render_pass.h"

using namespace llt;

Pipeline::Pipeline(VkShaderStageFlagBits stage)
	: m_stage(stage)
	, m_descriptorSetLayout(VK_NULL_HANDLE)
{
}

Pipeline::~Pipeline()
{
}

void Pipeline::setDescriptorSetLayout(const VkDescriptorSetLayout& layout)
{
	m_descriptorSetLayout = layout;
}

void Pipeline::preCache()
{
	getPipeline();
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

// --- graphics pipeline ---

GraphicsPipeline::GraphicsPipeline()
	: Pipeline(VK_SHADER_STAGE_ALL_GRAPHICS)
	, m_minSampleShading(0.2f)
	, m_shaderStages()
	, m_sampleShadingEnabled(true)
	, m_depthStencilCreateInfo()
	, m_viewport()
	, m_scissor()
	, m_cullMode(VK_CULL_MODE_BACK_BIT)
	, m_currentVertexFormat()
{
	resetViewport();
	resetScissor();

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

void GraphicsPipeline::render(const RenderPass& op)
{
	cauto& currentBuffer = g_vulkanBackend->getGraphicsCommandBuffer();

	VkViewport viewport = getViewport();
	VkRect2D scissor = getScissor();

	vkCmdSetViewport(currentBuffer, 0, 1, &viewport);
	vkCmdSetScissor(currentBuffer, 0, 1, &scissor);

	VkPipelineLayout pipelineLayout = getPipelineLayout();

	if (g_vulkanBackend->m_pushConstants.size() > 0)
	{
		vkCmdPushConstants(
			currentBuffer,
			pipelineLayout,
			VK_SHADER_STAGE_ALL_GRAPHICS,
			0,
			g_vulkanBackend->m_pushConstants.size(),
			g_vulkanBackend->m_pushConstants.data()
		);
	}

	cauto& vertexBuffer = op.vertexBuffer->getBuffer();
	cauto& indexBuffer  = op.indexBuffer->getBuffer();

	VkBuffer pVertexBuffers[] = {
		vertexBuffer,
		VK_NULL_HANDLE
	};

	int nVertexBuffers = 1;

	if (op.instanceBuffer)
	{
		pVertexBuffers[1] = op.instanceBuffer->getBuffer();
		nVertexBuffers = 2;
	}

	cauto& bindings = m_currentVertexFormat.getBindingDescriptions();

	VkDeviceSize vertexBufferOffsets[] = { 0, 0 };

	vkCmdBindVertexBuffers(
		currentBuffer,
		bindings[0].binding,
		nVertexBuffers,
		pVertexBuffers,
		vertexBufferOffsets
	);

	VkDeviceSize indexBufferOffset = 0;

	vkCmdBindIndexBuffer(
		currentBuffer,
		indexBuffer,
		indexBufferOffset,
		VK_INDEX_TYPE_UINT16
	);

	if (op.indirectBuffer != nullptr)
	{
		// todo: start using vkCmdDrawIndirectCount
		vkCmdDrawIndexedIndirect(
			currentBuffer,
			op.indirectBuffer->getBuffer(),
			op.indirectOffset,
			op.indirectOffset,
			sizeof(VkDrawIndexedIndirectCommand)
		);
	}
	else
	{
		vkCmdDrawIndexed(
			currentBuffer,
			op.nIndices,
			op.instanceCount,
			0,
			0,
			op.firstInstance
		);
	}
}

void GraphicsPipeline::bind()
{
	vkCmdBindPipeline(
		g_vulkanBackend->getGraphicsCommandBuffer(),
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		getPipeline()
	);
}

VkPipeline GraphicsPipeline::getPipeline()
{
	cauto& renderInfo = g_vulkanBackend->getRenderTarget()->getRenderInfo();
	cauto& rasterSamples = g_vulkanBackend->getRenderTarget()->getMSAA();

	cauto& bindingDescriptions = m_currentVertexFormat.getBindingDescriptions();
	cauto& attributeDescriptions = m_currentVertexFormat.getAttributeDescriptions();

	cauto& blendAttachments = renderInfo->getColourBlendAttachmentStates();

	VkViewport viewport = getViewport();
	VkRect2D scissor = getScissor();

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
	viewportStateCreateInfo.pViewports = &viewport;
	viewportStateCreateInfo.scissorCount = 1;
	viewportStateCreateInfo.pScissors = &scissor;

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
	multisampleStateCreateInfo.rasterizationSamples = rasterSamples;
	multisampleStateCreateInfo.pSampleMask = nullptr;
	multisampleStateCreateInfo.alphaToCoverageEnable = VK_FALSE;
	multisampleStateCreateInfo.alphaToOneEnable = VK_FALSE;

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

	hash::combine(&createdPipelineHash, &m_depthStencilCreateInfo);
	hash::combine(&createdPipelineHash, &colourBlendStateCreateInfo);
	hash::combine(&createdPipelineHash, &rasterizationStateCreateInfo);
	hash::combine(&createdPipelineHash, &inputAssemblyStateCreateInfo);
	hash::combine(&createdPipelineHash, &multisampleStateCreateInfo);
	hash::combine(&createdPipelineHash, &viewportStateCreateInfo);

	VkRenderingInfoKHR info = renderInfo->getInfo();

	hash::combine(&createdPipelineHash, &info);

	for (auto& attrib : attributeDescriptions) {
		hash::combine(&createdPipelineHash, &attrib);
	}

	for (auto& binding : bindingDescriptions) {
		hash::combine(&createdPipelineHash, &binding);
	}

	for (int i = 0; i < m_shaderStages.size(); i++) {
		hash::combine(&createdPipelineHash, &m_shaderStages[i]);
	}

	if (g_vulkanBackend->m_pipelineCache.contains(createdPipelineHash)) {
		return g_vulkanBackend->m_pipelineCache[createdPipelineHash];
	}

	VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
	dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicStateCreateInfo.dynamicStateCount = LLT_ARRAY_LENGTH(vkutil::DYNAMIC_STATES);
	dynamicStateCreateInfo.pDynamicStates = vkutil::DYNAMIC_STATES;

	cauto& colourFormats = renderInfo->getColourAttachmentFormats();

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

	VkPipeline createdPipeline = VK_NULL_HANDLE;

	LLT_VK_CHECK(
		vkCreateGraphicsPipelines(g_vulkanBackend->m_device, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &createdPipeline),
		"Failed to create new graphics pipeline"
	);

	LLT_LOG("Created new graphics pipeline!");

	g_vulkanBackend->m_pipelineCache.insert(
		createdPipelineHash,
		createdPipeline
	);

	return createdPipeline;
}

void GraphicsPipeline::bindShader(const ShaderProgram* shader)
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
			break;

		default:
			LLT_ERROR("Unrecognised graphics shader type being bound: %d", shader->type);
			break;
	}
}

void GraphicsPipeline::setVertexFormat(const VertexFormat& descriptor)
{
	m_currentVertexFormat = descriptor;
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

void GraphicsPipeline::setScissor(const RectI& rect)
{
	m_scissor = (VkRect2D) { rect.x, rect.y, (uint32_t)rect.w, (uint32_t)rect.h };
}

VkViewport GraphicsPipeline::getViewport() const
{
	RenderInfo* renderInfo = g_vulkanBackend->getRenderTarget()->getRenderInfo();

	VkViewport defaultViewport = {};
	defaultViewport.x = 0.0f;
	defaultViewport.y = (float)renderInfo->getHeight();
	defaultViewport.width = (float)renderInfo->getWidth();
	defaultViewport.height = -(float)renderInfo->getHeight();
	defaultViewport.minDepth = 0.0f;
	defaultViewport.maxDepth = 1.0f;

	return m_viewport.valueOr(defaultViewport);
}

VkRect2D GraphicsPipeline::getScissor() const
{
	RenderInfo* renderInfo = g_vulkanBackend->getRenderTarget()->getRenderInfo();

	VkRect2D defaultScissor = {};
	defaultScissor.offset = { 0, 0 };
	defaultScissor.extent = { renderInfo->getWidth(), renderInfo->getHeight() };

	return m_scissor.valueOr(defaultScissor);
}

void GraphicsPipeline::resetViewport()
{
	m_viewport.disable();
}

void GraphicsPipeline::resetScissor()
{
	m_scissor.disable();
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

// --- compute pipeline ---

ComputePipeline::ComputePipeline()
	: Pipeline(VK_SHADER_STAGE_COMPUTE_BIT)
	, m_computeShaderStageInfo()
{
}

ComputePipeline::~ComputePipeline()
{
}

void ComputePipeline::dispatch(int gcX, int gcY, int gcZ)
{
	//p_boundTextures.pushPipelineBarriers(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

	auto currentBuffer = g_vulkanBackend->getComputeCommandBuffer();

	VkPipelineLayout pipelineLayout = getPipelineLayout();

	if (g_vulkanBackend->m_pushConstants.size() > 0)
	{
		vkCmdPushConstants(
			currentBuffer,
			pipelineLayout,
			VK_SHADER_STAGE_COMPUTE_BIT,
			0,
			g_vulkanBackend->m_pushConstants.size(),
			g_vulkanBackend->m_pushConstants.data()
		);
	}

	vkCmdDispatch(
		currentBuffer,
		gcX, gcY, gcZ
	);

	//p_boundTextures.popPipelineBarriers();
}

void ComputePipeline::bind()
{
	vkCmdBindPipeline(
		g_vulkanBackend->getComputeCommandBuffer(),
		VK_PIPELINE_BIND_POINT_COMPUTE,
		getPipeline()
	);
}

VkPipeline ComputePipeline::getPipeline()
{
	VkPipelineLayout layout = getPipelineLayout();

	uint64_t createdPipelineHash = 0;

	hash::combine(&createdPipelineHash, &m_computeShaderStageInfo);
	hash::combine(&createdPipelineHash, &layout);

	if (g_vulkanBackend->m_pipelineCache.contains(createdPipelineHash)) {
		return g_vulkanBackend->m_pipelineCache[createdPipelineHash];
	}

	VkComputePipelineCreateInfo computePipelineCreateInfo = {};
	computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	computePipelineCreateInfo.layout = layout;
	computePipelineCreateInfo.stage = m_computeShaderStageInfo;

	VkPipeline createdPipeline = VK_NULL_HANDLE;

	LLT_VK_CHECK(
		vkCreateComputePipelines(g_vulkanBackend->m_device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &createdPipeline),
		"Failed to create new compute pipeline"
	);

	g_vulkanBackend->m_pipelineCache.insert(
		createdPipelineHash,
		createdPipeline
	);

	LLT_LOG("Created new compute pipeline!");

	return createdPipeline;
}

void ComputePipeline::bindShader(const ShaderProgram* shader)
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
