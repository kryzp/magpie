#include "shader.h"
#include "backend.h"
#include "util.h"

using namespace llt;

const ShaderParameters::PackedData &ShaderParameters::getPackedConstants()
{
	if (m_dirtyConstants) {
		rebuildPackedConstantData();
		m_dirtyConstants = false;
	}

	return m_packedConstants;
}

void ShaderParameters::rebuildPackedConstantData()
{
	m_packedConstants.resize(m_size);

	for (auto& [name, param] : m_constants) {
		mem::copy(m_packedConstants.data() + param.offset, param.data, param.size);
	}
}

// --- //

ShaderProgram::ShaderProgram()
	: m_stage()
	, m_module(VK_NULL_HANDLE)
{
}

ShaderProgram::~ShaderProgram()
{
	cleanUp();
}

void ShaderProgram::cleanUp()
{
	if (m_module == VK_NULL_HANDLE) {
		return;
	}

	vkDestroyShaderModule(g_vulkanBackend->m_device, m_module, nullptr);
	m_module = VK_NULL_HANDLE;
}

void ShaderProgram::loadFromSource(const char *source, uint64_t size)
{
	// create the shader module from source
	VkShaderModuleCreateInfo moduleCreateInfo = {};
	moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	moduleCreateInfo.codeSize = size;
	moduleCreateInfo.pCode = (const uint32_t *)source;

	LLT_VK_CHECK(
		vkCreateShaderModule(g_vulkanBackend->m_device, &moduleCreateInfo, nullptr, &m_module),
		"Failed to create shader module"
	);
}

VkPipelineShaderStageCreateInfo ShaderProgram::getShaderStageCreateInfo() const
{
	VkPipelineShaderStageCreateInfo shaderStage = {};
	shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStage.stage = m_stage;
	shaderStage.module = m_module;
	shaderStage.pName = "main";

	return shaderStage;
}

VkShaderStageFlagBits ShaderProgram::getStage() const
{
	return m_stage;
}

void ShaderProgram::setStage(VkShaderStageFlagBits stage)
{
	m_stage = stage;
}

VkShaderModule ShaderProgram::getModule() const
{
	return m_module;
}

// ============================================= //

ShaderEffect::ShaderEffect()
	: m_stages()
	, m_layout()
	, m_descriptorSetLayout()
	, m_pushConstantsSize(0)
{
}

VkPipelineLayout ShaderEffect::getPipelineLayout()
{
	VkShaderStageFlagBits shaderStage = m_stages[0]->getStage() == VK_SHADER_STAGE_COMPUTE_BIT ? VK_SHADER_STAGE_COMPUTE_BIT : VK_SHADER_STAGE_ALL_GRAPHICS;

	uint64_t pipelineLayoutHash = 0;

	VkPushConstantRange pushConstants = {};
	pushConstants.offset = 0;
	pushConstants.size = m_pushConstantsSize;
	pushConstants.stageFlags = shaderStage;

	hash::combine(&pipelineLayoutHash, &pushConstants);
	hash::combine(&pipelineLayoutHash, &m_descriptorSetLayout);

	if (g_vulkanBackend->m_pipelineLayoutCache.contains(pipelineLayoutHash))
	{
		m_layout = g_vulkanBackend->m_pipelineLayoutCache[pipelineLayoutHash];
		return m_layout;
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

	LLT_VK_CHECK(
		vkCreatePipelineLayout(g_vulkanBackend->m_device, &pipelineLayoutCreateInfo, nullptr, &m_layout),
		"Failed to create pipeline layout"
	);

	g_vulkanBackend->m_pipelineLayoutCache.insert(
		pipelineLayoutHash,
		m_layout
	);

	LLT_LOG("Created new pipeline layout!");

	return m_layout;
}

void ShaderEffect::addStage(ShaderProgram *program)
{
	m_stages.pushBack(program);
}

const Vector<ShaderProgram *> &ShaderEffect::getStages() const
{
	return m_stages;
}

const VkDescriptorSetLayout &ShaderEffect::getDescriptorSetLayout() const
{
	return m_descriptorSetLayout;
}

void ShaderEffect::setDescriptorSetLayout(const VkDescriptorSetLayout &layout)
{
	m_descriptorSetLayout = layout;
}

void ShaderEffect::setPushConstantsSize(uint64_t size)
{
	m_pushConstantsSize = size;
}
