#include "graphics_pipeline.h"

#include "backend.h"
#include "util.h"
#include "shader.h"
#include "render_target.h"
#include "render_pass.h"

using namespace llt;

GraphicsPipeline::GraphicsPipeline()
	: Pipeline(VK_SHADER_STAGE_ALL_GRAPHICS)
	, m_minSampleShading(0.2f)
	, m_blendStateLogicOpEnabled(false)
	, m_blendStateLogicOp(VK_LOGIC_OP_COPY)
	, m_graphicsShaderStages()
	, m_sampleShadingEnabled(true)
	, m_depthStencilCreateInfo()
	, m_colourBlendAttachmentStates()
	, m_blendConstants{0.0f, 0.0f, 0.0f, 0.0f}
	, m_viewport()
	, m_scissor()
	, m_cullMode(VK_CULL_MODE_BACK_BIT)
	, m_currentVertexDescriptor(nullptr)
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

	for (int i = 0; i < m_colourBlendAttachmentStates.size(); i++)
	{
		m_colourBlendAttachmentStates[i].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		m_colourBlendAttachmentStates[i].blendEnable = VK_TRUE;
		m_colourBlendAttachmentStates[i].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		m_colourBlendAttachmentStates[i].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		m_colourBlendAttachmentStates[i].colorBlendOp = VK_BLEND_OP_ADD;
		m_colourBlendAttachmentStates[i].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		m_colourBlendAttachmentStates[i].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		m_colourBlendAttachmentStates[i].alphaBlendOp = VK_BLEND_OP_ADD;
	}
}

GraphicsPipeline::~GraphicsPipeline()
{
}

void GraphicsPipeline::render(const RenderOp& op)
{
	auto currentFrame = g_vulkanBackend->graphicsQueue.getCurrentFrame();
	auto currentBuffer = currentFrame.commandBuffer;

	const auto& vertexBuffer = op.meshData.vBuffer->getBuffer();
	const auto& indexBuffer  = op.meshData.iBuffer->getBuffer();

	VkPipelineLayout pipelineLayout = getPipelineLayout();
	VkDescriptorSet descriptorSet = getDescriptorSet();

	VkViewport viewport = getViewport();
	VkRect2D scissor = getScissor();

	vkCmdSetViewport(currentBuffer, 0, 1, &viewport);
	vkCmdSetScissor(currentBuffer, 0, 1, &scissor);

	VkBuffer vertexBuffers[2] = { vertexBuffer, VK_NULL_HANDLE };

	if (op.instanceData.buffer)
	{
		vertexBuffers[1] = op.instanceData.buffer->getBuffer();
	}

	VkDeviceSize offsets[] = { 0, 0 };

	const auto& bindings = m_currentVertexDescriptor->getBindingDescriptions();

	for (int i = 0; i < bindings.size(); i++)
	{
		vkCmdBindVertexBuffers(currentBuffer, bindings[i].binding, 1, &vertexBuffers[i], offsets);
	}

	vkCmdBindIndexBuffer(currentBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);

	if (g_vulkanBackend->pushConstants.size() > 0)
	{
		vkCmdPushConstants(
			currentBuffer,
			pipelineLayout,
			VK_SHADER_STAGE_ALL_GRAPHICS,
			0,
			g_vulkanBackend->pushConstants.size(),
			g_vulkanBackend->pushConstants.data()
		);
	}

	Vector<uint32_t> dynamicOffsets = getDynamicOffsets();

	vkCmdBindDescriptorSets(
		currentBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		pipelineLayout,
		0,
		1, &descriptorSet,
		dynamicOffsets.size(),
		dynamicOffsets.data()
	);

	if (op.indirectData.buffer)
	{
		// todo: start using vkCmdDrawIndirectCount
		vkCmdDrawIndexedIndirect(
			currentBuffer,
			op.indirectData.buffer->getBuffer(),
			op.indirectData.offset,
			op.indirectData.drawCount,
			sizeof(VkDrawIndexedIndirectCommand)
		);
	}
	else
	{
		vkCmdDrawIndexed(
			currentBuffer,
			op.meshData.nIndices,
			op.instanceData.instanceCount,
			0,
			0,
			op.instanceData.firstInstance
		);
	}
}

void GraphicsPipeline::bind()
{
	auto currentFrame = g_vulkanBackend->graphicsQueue.getCurrentFrame();
	auto currentBuffer = currentFrame.commandBuffer;

	VkPipeline pipeline = getPipeline();

	vkCmdBindPipeline(
		currentBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		pipeline
	);
}

VkPipeline GraphicsPipeline::getPipeline()
{
	auto renderInfoBuilder = g_vulkanBackend->getRenderTarget()->getRenderInfo();
	auto rasterSamples = g_vulkanBackend->getRenderTarget()->getMSAA();

	const auto& bindingDescriptions = m_currentVertexDescriptor->getBindingDescriptions();
	const auto& attributeDescriptions = m_currentVertexDescriptor->getAttributeDescriptions();

	auto vport = getViewport();
	auto scissor = getScissor();

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
	viewportStateCreateInfo.pViewports = &vport;
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
	colourBlendStateCreateInfo.logicOpEnable = m_blendStateLogicOpEnabled ? VK_TRUE : VK_FALSE;
	colourBlendStateCreateInfo.logicOp = m_blendStateLogicOp;
	colourBlendStateCreateInfo.attachmentCount = renderInfoBuilder->getColourAttachmentCount();
	colourBlendStateCreateInfo.pAttachments = m_colourBlendAttachmentStates.data();
	colourBlendStateCreateInfo.blendConstants[0] = m_blendConstants[0];
	colourBlendStateCreateInfo.blendConstants[1] = m_blendConstants[1];
	colourBlendStateCreateInfo.blendConstants[2] = m_blendConstants[2];
	colourBlendStateCreateInfo.blendConstants[3] = m_blendConstants[3];

	uint64_t createdPipelineHash = 0;

	hash::combine(&createdPipelineHash, &m_depthStencilCreateInfo);
	hash::combine(&createdPipelineHash, m_colourBlendAttachmentStates.data());
	hash::combine(&createdPipelineHash, &m_blendConstants);
	hash::combine(&createdPipelineHash, &rasterizationStateCreateInfo);
	hash::combine(&createdPipelineHash, &inputAssemblyStateCreateInfo);
	hash::combine(&createdPipelineHash, &multisampleStateCreateInfo);

	VkRenderingInfoKHR info = renderInfoBuilder->buildInfo();

	hash::combine(&createdPipelineHash, &info);

	for (auto& attrib : attributeDescriptions) {
		hash::combine(&createdPipelineHash, &attrib);
	}

	for (auto& binding : bindingDescriptions) {
		hash::combine(&createdPipelineHash, &binding);
	}

	for (int i = 0; i < m_graphicsShaderStages.size(); i++) {
		hash::combine(&createdPipelineHash, &m_graphicsShaderStages[i]);
	}

	if (g_vulkanBackend->pipelineCache.contains(createdPipelineHash)) {
		return g_vulkanBackend->pipelineCache[createdPipelineHash];
	}

	VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
	dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicStateCreateInfo.dynamicStateCount = LLT_ARRAY_LENGTH(vkutil::DYNAMIC_STATES);
	dynamicStateCreateInfo.pDynamicStates = vkutil::DYNAMIC_STATES;

	auto colourFormats = renderInfoBuilder->getColourAttachmentFormats();

	VkPipelineRenderingCreateInfo pipelineRenderingCreateInfo = {};
	pipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
	pipelineRenderingCreateInfo.colorAttachmentCount = colourFormats.size();
	pipelineRenderingCreateInfo.pColorAttachmentFormats = colourFormats.data();
	pipelineRenderingCreateInfo.depthAttachmentFormat = vkutil::findDepthFormat(g_vulkanBackend->physicalData.device);
	pipelineRenderingCreateInfo.stencilAttachmentFormat = vkutil::findDepthFormat(g_vulkanBackend->physicalData.device);

	VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {};
	graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	graphicsPipelineCreateInfo.pStages = m_graphicsShaderStages.data();
	graphicsPipelineCreateInfo.stageCount = m_graphicsShaderStages.size();
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

	if (VkResult result = vkCreateGraphicsPipelines(g_vulkanBackend->device, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &createdPipeline); result != VK_SUCCESS) {
		LLT_ERROR("[VULKAN|DEBUG] Failed to create new graphics pipeline: %d", result);
	}

	LLT_LOG("[VULKAN] Created new graphics pipeline!");
		
	g_vulkanBackend->pipelineCache.insert(
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
			m_graphicsShaderStages[0] = shader->getShaderStageCreateInfo();
			break;

		case VK_SHADER_STAGE_FRAGMENT_BIT:
			m_graphicsShaderStages[1] = shader->getShaderStageCreateInfo();
			break;

		case VK_SHADER_STAGE_GEOMETRY_BIT:
			LLT_ERROR("[GRAPHICSPIPELINE|DEBUG] Geometry shaders not yet implemented!!");
			break;

		default:
			LLT_ERROR("[GRAPHICSPIPELINE|DEBUG] Unrecognised shader type being bound: %d", shader->type);
			break;
	}
}

void GraphicsPipeline::setVertexDescriptor(const VertexDescriptor& descriptor)
{
	m_currentVertexDescriptor = &descriptor;
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
	m_viewport = (VkViewport){ rect.x, rect.y, rect.w, rect.h };
}

void GraphicsPipeline::setScissor(const RectI& rect)
{
	m_scissor = (VkRect2D){ rect.x, rect.y, (uint32_t)rect.w, (uint32_t)rect.h };
}

VkViewport GraphicsPipeline::getViewport() const
{
	auto renderInfoBuilder = g_vulkanBackend->getRenderTarget()->getRenderInfo();

	VkViewport defaultValue = {};
	defaultValue.x = 0.0f;
	defaultValue.y = (float)renderInfoBuilder->getHeight();
	defaultValue.width = (float)renderInfoBuilder->getWidth();
	defaultValue.height = -(float)renderInfoBuilder->getHeight();
	defaultValue.minDepth = 0.0f;
	defaultValue.maxDepth = 1.0f;

	return m_viewport.valueOr(defaultValue);
}

VkRect2D GraphicsPipeline::getScissor() const
{
	auto renderInfoBuilder = g_vulkanBackend->getRenderTarget()->getRenderInfo();

	VkRect2D defaultValue = {};
	defaultValue.offset = { 0, 0 };
	defaultValue.extent = { renderInfoBuilder->getWidth(), renderInfoBuilder->getHeight() };

	return m_scissor.valueOr(defaultValue);
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

void GraphicsPipeline::setBlendState(const BlendState& state)
{
	m_blendConstants[0] = state.blendConstants[0];
	m_blendConstants[1] = state.blendConstants[1];
	m_blendConstants[2] = state.blendConstants[2];
	m_blendConstants[3] = state.blendConstants[3];

	m_blendStateLogicOpEnabled = state.blendOpEnabled;
	m_blendStateLogicOp = state.blendOp;

	for (int i = 0; i < m_colourBlendAttachmentStates.size(); i++)
	{
		m_colourBlendAttachmentStates[i].colorWriteMask = 0;

		if (state.writeMask[0]) m_colourBlendAttachmentStates[i].colorWriteMask |= VK_COLOR_COMPONENT_R_BIT;
		if (state.writeMask[1]) m_colourBlendAttachmentStates[i].colorWriteMask |= VK_COLOR_COMPONENT_G_BIT;
		if (state.writeMask[2]) m_colourBlendAttachmentStates[i].colorWriteMask |= VK_COLOR_COMPONENT_B_BIT;
		if (state.writeMask[3]) m_colourBlendAttachmentStates[i].colorWriteMask |= VK_COLOR_COMPONENT_A_BIT;

		m_colourBlendAttachmentStates[i].colorBlendOp = state.colour.op;
		m_colourBlendAttachmentStates[i].srcColorBlendFactor = state.colour.src;
		m_colourBlendAttachmentStates[i].dstColorBlendFactor = state.colour.dst;

		m_colourBlendAttachmentStates[i].alphaBlendOp = state.alpha.op;
		m_colourBlendAttachmentStates[i].srcAlphaBlendFactor = state.alpha.src;
		m_colourBlendAttachmentStates[i].dstAlphaBlendFactor = state.alpha.dst;

		m_colourBlendAttachmentStates[i].blendEnable = state.enabled ? VK_TRUE : VK_FALSE;
	}
}
