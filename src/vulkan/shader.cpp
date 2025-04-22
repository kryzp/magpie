#include "shader.h"

#include "core/common.h"

#include "core.h"

using namespace mgp;

ShaderStage::ShaderStage(const VulkanCore *core)
	: m_stage()
	, m_module(VK_NULL_HANDLE)
	, m_core(core)
{
}

ShaderStage::ShaderStage(const VulkanCore *core, VkShaderStageFlagBits stage, const char *source, uint32_t size)
	: m_stage(stage)
	, m_module(VK_NULL_HANDLE)
	, m_core(core)
{
	loadFromSource(source, size);
}

ShaderStage::~ShaderStage()
{
	vkDestroyShaderModule(m_core->getLogicalDevice(), m_module, nullptr);
	m_module = VK_NULL_HANDLE;
}

void ShaderStage::loadFromSource(const char *source, uint64_t size)
{
	VkShaderModuleCreateInfo moduleCreateInfo = {};
	moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	moduleCreateInfo.codeSize = size;
	moduleCreateInfo.pCode = (const uint32_t *)source;

	MGP_VK_CHECK(
		vkCreateShaderModule(m_core->getLogicalDevice(), &moduleCreateInfo, nullptr, &m_module),
		"Failed to create shader module"
	);
}

VkPipelineShaderStageCreateInfo ShaderStage::getShaderStageCreateInfo() const
{
	VkPipelineShaderStageCreateInfo shaderStage = {};
	shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStage.stage = m_stage;
	shaderStage.module = m_module;
	shaderStage.pName = "main";

	return shaderStage;
}

VkShaderStageFlagBits ShaderStage::getStage() const
{
	return m_stage;
}

void ShaderStage::setStage(VkShaderStageFlagBits stage)
{
	m_stage = stage;
}

VkShaderModule ShaderStage::getModule() const
{
	return m_module;
}

Shader::Shader()
	: m_stages()
	, m_descriptorSetLayouts()
	, m_pushConstantsSize(0)
{
}

void Shader::addStage(ShaderStage *stage)
{
	m_stages.push_back(stage);
}

const std::vector<ShaderStage *> &Shader::getStages() const
{
	return m_stages;
}

const ShaderStage *Shader::getStage(int idx) const
{
	return m_stages[idx];
}

const std::vector<VkDescriptorSetLayout> &Shader::getDescriptorSetLayouts() const
{
	return m_descriptorSetLayouts;
}

void Shader::setDescriptorSetLayouts(const std::vector<VkDescriptorSetLayout> &layouts)
{
	m_descriptorSetLayouts = layouts;
}

void Shader::setPushConstantsSize(uint64_t size)
{
	m_pushConstantsSize = size;
}

uint64_t Shader::getPushConstantsSize() const
{
	return m_pushConstantsSize;
}
