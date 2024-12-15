#include "compute_pipeline.h"

#include "../common.h"

#include "backend.h"

using namespace llt;

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
	p_textureBatch.pushPipelineBarriers(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

	auto currentBuffer = g_vulkanBackend->computeQueues[0].getCurrentFrame().commandBuffer;

	VkPipeline pipeline = getPipeline();
	VkPipelineLayout pipelineLayout = getPipelineLayout();
	VkDescriptorSet descriptorSet = getDescriptorSet();

	if (g_vulkanBackend->pushConstants.size() > 0)
	{
		vkCmdPushConstants(
			currentBuffer,
			pipelineLayout,
			VK_SHADER_STAGE_COMPUTE_BIT,
			0,
			g_vulkanBackend->pushConstants.size(),
			g_vulkanBackend->pushConstants.data()
		);
	}

	Vector<uint32_t> dynamicOffsets = getDynamicOffsets();

	vkCmdBindDescriptorSets(
		currentBuffer,
		VK_PIPELINE_BIND_POINT_COMPUTE,
		pipelineLayout,
		0,
		1, &descriptorSet,
		dynamicOffsets.size(),
		dynamicOffsets.data()
	);

	vkCmdBindPipeline(
		currentBuffer,
		VK_PIPELINE_BIND_POINT_COMPUTE,
		pipeline
	);

	vkCmdDispatch(
		currentBuffer,
		gcX, gcY, gcZ
	);

	p_textureBatch.popPipelineBarriers();
}

VkPipeline ComputePipeline::getPipeline()
{
	VkPipelineLayout layout = getPipelineLayout();

	uint64_t createdPipelineHash = 0;

	hash::combine(&createdPipelineHash, &m_computeShaderStageInfo);
	hash::combine(&createdPipelineHash, &layout);

	if (g_vulkanBackend->pipelineCache.contains(createdPipelineHash)) {
		return g_vulkanBackend->pipelineCache[createdPipelineHash];
	}

	VkComputePipelineCreateInfo computePipelineCreateInfo = {};
	computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	computePipelineCreateInfo.layout = layout;
	computePipelineCreateInfo.stage = m_computeShaderStageInfo;

	VkPipeline createdPipeline = VK_NULL_HANDLE;

	if (VkResult result = vkCreateComputePipelines(g_vulkanBackend->device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &createdPipeline); result != VK_SUCCESS) {
		LLT_ERROR("[VULKAN|DEBUG] Failed to create new compute pipeline: %d", result);
	}

	g_vulkanBackend->pipelineCache.insert(
		createdPipelineHash,
		createdPipeline
	);

	LLT_LOG("[VULKAN] Created new compute pipeline!");

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
			LLT_ERROR("[COMPUTEPIPELINE|DEBUG] Unrecognised shader type being bound: %d", shader->type);
			break;
	}
}
