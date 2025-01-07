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

VkPipelineLayout Pipeline::getPipelineLayout()
{
	uint64_t pipelineLayoutHash = 0;

	hash::combine(&pipelineLayoutHash, &m_descriptorSetLayout);

	VkPushConstantRange pushConstants = {};
	pushConstants.offset = 0;
	pushConstants.size = g_vulkanBackend->pushConstants.size();
	pushConstants.stageFlags = m_stage;

	hash::combine(&pipelineLayoutHash, &pushConstants);

	if (g_vulkanBackend->pipelineLayoutCache.contains(pipelineLayoutHash)) {
		return g_vulkanBackend->pipelineLayoutCache[pipelineLayoutHash];
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
		vkCreatePipelineLayout(g_vulkanBackend->device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout),
		"Failed to create pipeline layout"
	);

	g_vulkanBackend->pipelineLayoutCache.insert(
		pipelineLayoutHash,
		pipelineLayout
	);

	LLT_LOG("Created new pipeline layout!");

	return pipelineLayout;
}
