#include "pipeline.h"

#include "core/common.h"

#include "graphics_core.h"
#include "shader.h"
#include "render_info.h"

using namespace mgp;

GraphicsPipelineDef::GraphicsPipelineDef()
	: m_shader(nullptr)
	, m_vertexFormat(nullptr)
	, m_cullMode(VK_CULL_MODE_BACK_BIT)
	, m_frontFace(VK_FRONT_FACE_CLOCKWISE)
	, m_depthStencilState()
	, m_blendConstants{ 0.0f, 0.0f, 0.0f, 0.0f }
	, m_colourBlendState()
	, m_blendStateLogicOpEnabled(false)
	, m_blendStateLogicOp()
	, m_sampleShadingEnabled(true)
	, m_minSampleShading(0.2f)
{
	m_depthStencilState.depthTestEnable = true;
	m_depthStencilState.depthWriteEnable = true;
	m_depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS;
	m_depthStencilState.depthBoundsTestEnable = false;
	m_depthStencilState.minDepthBounds = 0.0f;
	m_depthStencilState.maxDepthBounds = 1.0f;
	m_depthStencilState.stencilTestEnable = false;
	m_depthStencilState.front = {};
	m_depthStencilState.back = {};
	
	m_colourBlendState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	m_colourBlendState.blendEnable = VK_TRUE;
	m_colourBlendState.colorBlendOp = VK_BLEND_OP_ADD;
	m_colourBlendState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	m_colourBlendState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	m_colourBlendState.alphaBlendOp = VK_BLEND_OP_ADD;
	m_colourBlendState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	m_colourBlendState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
}

void GraphicsPipelineDef::setShader(const Shader *shader)
{
	m_shader = shader;
}

const Shader *GraphicsPipelineDef::getShader() const
{
	return m_shader;
}

void GraphicsPipelineDef::setVertexFormat(const VertexFormat *format)
{
	m_vertexFormat = format;
}

const VertexFormat *GraphicsPipelineDef::getVertexFormat() const
{
	return m_vertexFormat;
}

void GraphicsPipelineDef::setSampleShading(bool enabled, float minSampleShading)
{
	m_sampleShadingEnabled = enabled;
	m_minSampleShading = minSampleShading;
}

bool GraphicsPipelineDef::isSampleShadingEnabled() const
{
	return m_sampleShadingEnabled;
}

float GraphicsPipelineDef::getMinSampleShading() const
{
	return m_minSampleShading;
}

void GraphicsPipelineDef::setCullMode(VkCullModeFlags cull)
{
	m_cullMode = cull;
}

VkCullModeFlags GraphicsPipelineDef::getCullMode() const
{
	return m_cullMode;
}

void GraphicsPipelineDef::setFrontFace(VkFrontFace front)
{
	m_frontFace = front;
}

VkFrontFace GraphicsPipelineDef::getFrontFace() const
{
	return m_frontFace;
}

void GraphicsPipelineDef::setDepthOp(VkCompareOp op)
{
	m_depthStencilState.depthCompareOp = op;
}

void GraphicsPipelineDef::setDepthTest(bool enabled)
{
	m_depthStencilState.depthTestEnable = enabled;
}

void GraphicsPipelineDef::setDepthWrite(bool enabled)
{
	m_depthStencilState.depthWriteEnable = enabled;
}

void GraphicsPipelineDef::setDepthBounds(float min, float max)
{
	m_depthStencilState.minDepthBounds = min;
	m_depthStencilState.maxDepthBounds = max;
}

void GraphicsPipelineDef::setDepthStencilTest(bool enabled)
{
	m_depthStencilState.stencilTestEnable = enabled;
}

void GraphicsPipelineDef::setBlendState(const BlendState &state)
{
	m_blendConstants[0] = state.blendConstants[0];
	m_blendConstants[1] = state.blendConstants[1];
	m_blendConstants[2] = state.blendConstants[2];
	m_blendConstants[3] = state.blendConstants[3];

	m_blendStateLogicOpEnabled = state.blendOpEnabled;
	m_blendStateLogicOp = state.blendOp;

	m_colourBlendState.colorWriteMask = 0;

	if (state.writeMask[0]) m_colourBlendState.colorWriteMask |= VK_COLOR_COMPONENT_R_BIT;
	if (state.writeMask[1]) m_colourBlendState.colorWriteMask |= VK_COLOR_COMPONENT_G_BIT;
	if (state.writeMask[2]) m_colourBlendState.colorWriteMask |= VK_COLOR_COMPONENT_B_BIT;
	if (state.writeMask[3]) m_colourBlendState.colorWriteMask |= VK_COLOR_COMPONENT_A_BIT;

	m_colourBlendState.colorBlendOp = state.colour.op;
	m_colourBlendState.srcColorBlendFactor = state.colour.src;
	m_colourBlendState.dstColorBlendFactor = state.colour.dst;

	m_colourBlendState.alphaBlendOp = state.alpha.op;
	m_colourBlendState.srcAlphaBlendFactor = state.alpha.src;
	m_colourBlendState.dstAlphaBlendFactor = state.alpha.dst;

	m_colourBlendState.blendEnable = state.enabled ? VK_TRUE : VK_FALSE;
}

const VkPipelineColorBlendAttachmentState &GraphicsPipelineDef::getColourBlendState() const
{
	return m_colourBlendState;
}

const VkPipelineDepthStencilStateCreateInfo &GraphicsPipelineDef::getDepthStencilState() const
{
	return m_depthStencilState;
}

bool GraphicsPipelineDef::isBlendStateLogicOpEnabled() const
{
	return m_blendStateLogicOpEnabled;
}

VkLogicOp GraphicsPipelineDef::getBlendStateLogicOp() const
{
	return m_blendStateLogicOp;
}

const std::array<float, 4> &GraphicsPipelineDef::getBlendConstants() const
{
	return m_blendConstants;
}

float GraphicsPipelineDef::getBlendConstant(int idx) const
{
	return m_blendConstants[idx];
}

uint64_t GraphicsPipelineDef::getHash() const
{
	uint64_t h = 0;

	hash::combine(&h, &m_shader);
	hash::combine(&h, &m_vertexFormat);
	hash::combine(&h, &m_cullMode);
	hash::combine(&h, &m_frontFace);
	hash::combine(&h, &m_depthStencilState);
	hash::combine(&h, &m_blendConstants);
	hash::combine(&h, &m_colourBlendState);
	hash::combine(&h, &m_blendStateLogicOpEnabled);
	hash::combine(&h, &m_blendStateLogicOp);
	hash::combine(&h, &m_sampleShadingEnabled);
	hash::combine(&h, &m_minSampleShading);

	return h;
}

ComputePipelineDef::ComputePipelineDef()
	: m_shader(nullptr)
{
}

void ComputePipelineDef::setShader(const Shader *shader)
{
	m_shader = shader;
}

const Shader *ComputePipelineDef::getShader() const
{
	return m_shader;
}

uint64_t ComputePipelineDef::getHash() const
{
	uint64_t h = 0;

	hash::combine(&h, &m_shader);

	return h;
}

void PipelineCache::init(GraphicsCore *gfx)
{
	m_gfx = gfx;
}

void PipelineCache::destroy()
{
	for (auto &[id, pipeline] : m_pipelines)
	{
		vkDestroyPipeline(m_gfx->getLogicalDevice(), pipeline, nullptr);
	}

	for (auto &[id, layout] : m_layouts)
	{
		vkDestroyPipelineLayout(m_gfx->getLogicalDevice(), layout, nullptr);
	}
	
	m_pipelines.clear();
	m_layouts.clear();
}

PipelineState PipelineCache::fetchGraphicsPipeline(const GraphicsPipelineDef &definition, const RenderInfo &renderInfo)
{
	uint32_t h1 = definition.getHash();
	uint32_t h2 = renderInfo.getHash();
	
	uint64_t createdPipelineHash = 0;

	hash::combine(&createdPipelineHash, &h1);
	hash::combine(&createdPipelineHash, &h2);

	VkPipelineLayout layout = fetchPipelineLayout(definition.getShader());

	if (m_pipelines.contains(createdPipelineHash))
	{
		PipelineState st = {};
		st.pipeline = m_pipelines[createdPipelineHash];
		st.layout = layout;

		return st;
	}

	VkPipeline pipeline = m_gfx->createGraphicsPipeline(layout, definition, renderInfo);
	
	m_pipelines.insert({
		createdPipelineHash,
		pipeline
	});

	PipelineState st = {};
	st.pipeline = pipeline;
	st.layout = layout;

	return st;
}

PipelineState PipelineCache::fetchComputePipeline(const ComputePipelineDef &definition)
{
	uint64_t createdPipelineHash = definition.getHash();

	VkPipelineLayout layout = fetchPipelineLayout(definition.getShader());

	if (m_pipelines.contains(createdPipelineHash))
	{
		PipelineState st = {};
		st.pipeline = m_pipelines[createdPipelineHash];
		st.layout = layout;

		return st;
	}

	VkPipeline pipeline = m_gfx->createComputePipeline(layout, definition);
	
	m_pipelines.insert({
		createdPipelineHash,
		pipeline
	});

	PipelineState st = {};
	st.pipeline = pipeline;
	st.layout = layout;

	return st;
}

VkPipelineLayout PipelineCache::fetchPipelineLayout(const Shader *shader)
{
	VkShaderStageFlags shaderStage = shader->getStages()[0]->getType() == VK_SHADER_STAGE_COMPUTE_BIT ? VK_SHADER_STAGE_COMPUTE_BIT : VK_SHADER_STAGE_ALL_GRAPHICS;
	uint64_t pcSize = shader->getPushConstantSize();

	cauto &setLayouts = shader->getLayouts();

	uint64_t pipelineLayoutHash = 0;

	hash::combine(&pipelineLayoutHash, &shaderStage);
	hash::combine(&pipelineLayoutHash, &pcSize);

	for (auto &layout : setLayouts)
	{
		hash::combine(&pipelineLayoutHash, &layout);
	}

	if (m_layouts.contains(pipelineLayoutHash))
	{
		return m_layouts[pipelineLayoutHash];
	}

	VkPipelineLayout layout = m_gfx->createPipelineLayout(shader);
	
	m_layouts.insert({
		pipelineLayoutHash,
		layout
	});

	return layout;
}
