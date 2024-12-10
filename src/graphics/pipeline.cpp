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
	: m_descriptorBuilder()
	, m_stage(stage)
	, m_boundImages()
	, p_boundImagesTextures()
	, m_buffers()
{
}

VkPipelineLayout Pipeline::getPipelineLayout()
{
	uint64_t pipelineLayoutHash = m_descriptorBuilder.hash();

	hash::combine(&pipelineLayoutHash, &m_stage);

	uint64_t pushConstantCount = g_vulkanBackend->pushConstants.size();
	hash::combine(&pipelineLayoutHash, &pushConstantCount);

	if (g_vulkanBackend->pipelineLayoutCache.contains(pipelineLayoutHash)) {
		return g_vulkanBackend->pipelineLayoutCache[pipelineLayoutHash];
	}

	VkDescriptorSetLayout layout = {};
	m_descriptorBuilder.buildLayout(layout);

	VkPushConstantRange pushConstants;
	pushConstants.offset = 0;
	pushConstants.size = g_vulkanBackend->pushConstants.size();
	pushConstants.stageFlags = m_stage;

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.setLayoutCount = 1;
	pipelineLayoutCreateInfo.pSetLayouts = &layout;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

	if (pushConstants.size > 0) {
		pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
		pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstants;
	}

	VkPipelineLayout pipelineLayout = {};

	if (VkResult result = vkCreatePipelineLayout(g_vulkanBackend->device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout); result != VK_SUCCESS) {
		LLT_ERROR("[PIPELINE] Failed to create pipeline layout: %d", result);
	}

	LLT_LOG("[PIPELINE] Created new pipeline layout!");

	g_vulkanBackend->pipelineLayoutCache.insert(
		pipelineLayoutHash,
		pipelineLayout
	);

	return pipelineLayout;
}

VkDescriptorSet Pipeline::getDescriptorSet()
{
	uint64_t descriptorSetHash = 0;

	//hash::combine(&descriptorSetHash, &m_stage);

	for (auto& info : m_boundImages)
	{
		hash::combine(&descriptorSetHash, &info);
	}

	VkDescriptorSet descriptorSet = {};
	VkDescriptorSetLayout descriptorSetLayout = {};
	
	m_descriptorBuilder.build(descriptorSet, descriptorSetLayout, descriptorSetHash);

	return descriptorSet;
}

Vector<uint32_t> Pipeline::getDynamicOffsets()
{
	// dynamic offsets must follow binding order!!

	Vector<Pair<uint32_t, uint32_t>> boundOffsets;

	for (int i = 0; i < m_buffers.size(); i++)
	{
		auto& buf = m_buffers[i];
		boundOffsets.pushBack(Pair<uint32_t, uint32_t>(buf.first, buf.second->getDynamicOffset()));
	}

	sortBoundOffsets(boundOffsets, 0, boundOffsets.size() - 1);

	// ---

	Vector<uint32_t> result;

	for (int i = 0; i < boundOffsets.size(); i++)
	{
		result.pushBack(boundOffsets[i].second);
	}

	return result;
}

// quicksort
void Pipeline::sortBoundOffsets(Vector<Pair<uint32_t, uint32_t>>& offsets, int lo, int hi)
{
	if (lo >= hi || lo < 0) {
		return;
	}

	int pivot = offsets[hi].first;
	int i = lo - 1;

	for (int j = lo; j < hi; j++)
	{
		if (offsets[j].first <= pivot)
		{
			i++;

			Pair<uint32_t, uint32_t> tmp = offsets[i];
			offsets[i] = offsets[j];
			offsets[j] = tmp;
		}
	}

	Pair<uint32_t, uint32_t> tmp = offsets[i + 1];
	offsets[i + 1] = offsets[hi];
	offsets[hi] = tmp;

	int partition = i + 1;

	sortBoundOffsets(offsets, lo, partition - 1);
	sortBoundOffsets(offsets, partition + 1, hi);
}

void Pipeline::bindTexture(int idx, const Texture* texture, TextureSampler* sampler)
{
	VkDescriptorImageInfo info = {};

	info.imageView = texture->getImageView();

	if (texture->isDepthTexture())
	{
		info.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
	}
	else
	{
		info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	}

	info.sampler = sampler->bind(4);

	m_boundImages.pushBack(info);
	p_boundImagesTextures.pushBack(texture);
	
	m_descriptorBuilder.bindImage(
		idx,
		&m_boundImages.back(),
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		m_stage
	);
}

void Pipeline::bindBuffer(int idx, const ShaderBuffer* buffer)
{
	m_buffers.pushBack(Pair(idx, buffer));

	m_descriptorBuilder.bindBuffer(
		idx,
		&buffer->getDescriptor(),
		buffer->getDescriptorType(),
		m_stage
	);
}
