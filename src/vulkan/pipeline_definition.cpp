#include "pipeline_definition.h"

#include "core.h"
#include "util.h"
#include "texture.h"
#include "shader.h"
#include "render_target.h"

using namespace llt;

GraphicsPipelineDefinition::GraphicsPipelineDefinition()
	: m_shader(nullptr)
	, m_vertexFormat(nullptr)
	, m_cullMode(VK_CULL_MODE_BACK_BIT)
	, m_shaderStages()
	, m_depthStencilInfo()
	, m_blendConstants{0.0f, 0.0f, 0.0f, 0.0f}
	, m_colourBlendState()
	, m_blendStateLogicOpEnabled(false)
	, m_blendStateLogicOp()
	, m_sampleShadingEnabled(true)
	, m_minSampleShading(0.2f)
{
	m_depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	m_depthStencilInfo.depthTestEnable = VK_TRUE;
	m_depthStencilInfo.depthWriteEnable = VK_TRUE;
	m_depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
	m_depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
	m_depthStencilInfo.minDepthBounds = 0.0f;
	m_depthStencilInfo.maxDepthBounds = 1.0f;
	m_depthStencilInfo.stencilTestEnable = VK_FALSE;
	m_depthStencilInfo.front = {};
	m_depthStencilInfo.back = {};

	m_colourBlendState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	m_colourBlendState.blendEnable = VK_TRUE;
	m_colourBlendState.colorBlendOp = VK_BLEND_OP_ADD;
	m_colourBlendState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	m_colourBlendState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	m_colourBlendState.alphaBlendOp = VK_BLEND_OP_ADD;
	m_colourBlendState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	m_colourBlendState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
}

void GraphicsPipelineDefinition::setShader(const ShaderEffect *shader)
{
	m_shader = shader;

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

			default:
				LLT_ERROR("Unrecognised shader stage being bound to pipeline: %d", stage->getStage());
		}
	}
}

const ShaderEffect *GraphicsPipelineDefinition::getShader() const
{
	return m_shader;
}

void GraphicsPipelineDefinition::setVertexFormat(const VertexFormat &format)
{
	m_vertexFormat = &format;
}

const VertexFormat &GraphicsPipelineDefinition::getVertexFormat() const
{
	return *m_vertexFormat;
}

void GraphicsPipelineDefinition::setSampleShading(bool enabled, float minSampleShading)
{
	m_sampleShadingEnabled = enabled;
	m_minSampleShading = minSampleShading;
}

bool GraphicsPipelineDefinition::isSampleShadingEnabled() const
{
	return m_sampleShadingEnabled;
}

float GraphicsPipelineDefinition::getMinSampleShading() const
{
	return m_minSampleShading;
}

void GraphicsPipelineDefinition::setCullMode(VkCullModeFlagBits cull)
{
	m_cullMode = cull;
}

VkCullModeFlagBits GraphicsPipelineDefinition::getCullMode() const
{
	return m_cullMode;
}

const Array<VkPipelineShaderStageCreateInfo, mgc::RASTER_SHADER_COUNT> &GraphicsPipelineDefinition::getShaderStages() const
{
	return m_shaderStages;
}

const VkPipelineShaderStageCreateInfo &GraphicsPipelineDefinition::getShaderStage(int idx) const
{
	return m_shaderStages[idx];
}

void GraphicsPipelineDefinition::setDepthOp(VkCompareOp op)
{
	m_depthStencilInfo.depthCompareOp = op;
}

void GraphicsPipelineDefinition::setDepthTest(bool enabled)
{
	m_depthStencilInfo.depthTestEnable = enabled ? VK_TRUE : VK_FALSE;
}

void GraphicsPipelineDefinition::setDepthWrite(bool enabled)
{
	m_depthStencilInfo.depthWriteEnable = enabled ? VK_TRUE : VK_FALSE;
}

void GraphicsPipelineDefinition::setDepthBounds(float min, float max)
{
	m_depthStencilInfo.minDepthBounds = min;
	m_depthStencilInfo.maxDepthBounds = max;
}

void GraphicsPipelineDefinition::setDepthStencilTest(bool enabled)
{
	m_depthStencilInfo.stencilTestEnable = enabled ? VK_TRUE : VK_FALSE;
}

const VkPipelineDepthStencilStateCreateInfo &GraphicsPipelineDefinition::getDepthStencilState() const
{
	return m_depthStencilInfo;
}

void GraphicsPipelineDefinition::setBlendState(const BlendState &state)
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

const VkPipelineColorBlendAttachmentState &GraphicsPipelineDefinition::getColourBlendState() const
{
	return m_colourBlendState;
}

bool GraphicsPipelineDefinition::isBlendStateLogicOpEnabled() const
{
	return m_blendStateLogicOpEnabled;
}

VkLogicOp GraphicsPipelineDefinition::getBlendStateLogicOp() const
{
	return m_blendStateLogicOp;
}

const Array<float, 4> &GraphicsPipelineDefinition::getBlendConstants() const
{
	return m_blendConstants;
}

float GraphicsPipelineDefinition::getBlendConstant(int idx) const
{
	return m_blendConstants[idx];
}

// ---

ComputePipelineDefinition::ComputePipelineDefinition()
	: m_shader(nullptr)
	, m_stage()
{
}

void ComputePipelineDefinition::setShader(const ShaderEffect *shader)
{
	m_shader = shader;
	m_stage = shader->getStage(0)->getShaderStageCreateInfo();
}

const ShaderEffect *ComputePipelineDefinition::getShader() const
{
	return m_shader;
}

const VkPipelineShaderStageCreateInfo &ComputePipelineDefinition::getStage() const
{
	return m_stage;
}
