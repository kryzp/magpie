#include "shader.h"
#include "core.h"
#include "util.h"

using namespace llt;

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

	vkDestroyShaderModule(g_vkCore->m_device, m_module, nullptr);
	m_module = VK_NULL_HANDLE;
}

void ShaderProgram::loadFromSource(const char *source, uint64_t size)
{
	VkShaderModuleCreateInfo moduleCreateInfo = {};
	moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	moduleCreateInfo.codeSize = size;
	moduleCreateInfo.pCode = (const uint32_t *)source;

	LLT_VK_CHECK(
		vkCreateShaderModule(g_vkCore->m_device, &moduleCreateInfo, nullptr, &m_module),
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

// ---

ShaderEffect::ShaderEffect()
	: m_stages()
	, m_descriptorSetLayouts()
	, m_pushConstantsSize(0)
{
}

void ShaderEffect::addStage(ShaderProgram *program)
{
	m_stages.pushBack(program);
}

const Vector<ShaderProgram *> &ShaderEffect::getStages() const
{
	return m_stages;
}

const ShaderProgram *ShaderEffect::getStage(int idx) const
{
	return m_stages[idx];
}

const Vector<VkDescriptorSetLayout> &ShaderEffect::getDescriptorSetLayouts() const
{
	return m_descriptorSetLayouts;
}

void ShaderEffect::setDescriptorSetLayouts(const Vector<VkDescriptorSetLayout> &layouts)
{
	m_descriptorSetLayouts = layouts;
}

void ShaderEffect::setPushConstantsSize(uint64_t size)
{
	m_pushConstantsSize = size;
}

uint64_t ShaderEffect::getPushConstantsSize() const
{
	return m_pushConstantsSize;
}
