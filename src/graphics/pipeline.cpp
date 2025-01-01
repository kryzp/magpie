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
	, p_boundTextures()
	, m_dynamicOffsets()
	, m_buffers()
{
}

VkPipelineLayout Pipeline::getPipelineLayout()
{
	uint64_t pipelineLayoutHash = m_descriptorBuilder.getHash();

	VkPushConstantRange pushConstants = {};
	pushConstants.offset = 0;
	pushConstants.size = g_vulkanBackend->pushConstants.size();
	pushConstants.stageFlags = m_stage;

	hash::combine(&pipelineLayoutHash, &pushConstants);

	if (g_vulkanBackend->pipelineLayoutCache.contains(pipelineLayoutHash)) {
		return g_vulkanBackend->pipelineLayoutCache[pipelineLayoutHash];
	}

	VkDescriptorSetLayout layout = {};
	m_descriptorBuilder.buildLayout(layout);

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.setLayoutCount = 1;
	pipelineLayoutCreateInfo.pSetLayouts = &layout;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

	if (pushConstants.size > 0)
	{
		pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
		pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstants;
	}

	VkPipelineLayout pipelineLayout = {};

	if (VkResult result = vkCreatePipelineLayout(g_vulkanBackend->device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout); result != VK_SUCCESS) {
		LLT_ERROR("[PIPELINE] Failed to create pipeline layout: %d", result);
	}

	g_vulkanBackend->pipelineLayoutCache.insert(
		pipelineLayoutHash,
		pipelineLayout
	);

	LLT_LOG("[PIPELINE] Created new pipeline layout!");

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

// quicksort
/*
void sortBoundOffsets(Vector<Pair<int, uint32_t>>& offsets, int lo, int hi)
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

			auto tmp = offsets[i];
			offsets[i] = offsets[j];
			offsets[j] = tmp;
		}
	}

	auto tmp = offsets[i + 1];
	offsets[i + 1] = offsets[hi];
	offsets[hi] = tmp;

	int partition = i + 1;

	sortBoundOffsets(offsets, lo, partition - 1);
	sortBoundOffsets(offsets, partition + 1, hi);
}
*/

const Vector<uint32_t>& Pipeline::getDynamicOffsets()
{
	m_dynamicOffsets.resize(m_buffers.size());

	for (int i = 0; i < m_buffers.size(); i++)
	{
		m_dynamicOffsets[i] = m_buffers[i].second->getDynamicOffset();
	}

	return m_dynamicOffsets;
}

void Pipeline::bindTexture(int idx, Texture* texture, TextureSampler* sampler)
{
	VkDescriptorImageInfo info = {};

	info.imageView = texture->getImageView();

	info.imageLayout = texture->isDepthTexture()
		? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
		: VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	info.sampler = sampler->bind(mgc::MAX_SAMPLER_MIP_LEVELS);

	m_boundImages.pushBack(info);
	p_boundTextures.addTexture(texture);

	m_descriptorBuilder.bindImage(
		idx,
		&m_boundImages.back(),
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		m_stage,
		1
	);
}

void Pipeline::bindBuffer(int idx, const ShaderBuffer* buffer)
{
	bool inserted = false;

	for (int i = 0; i < m_buffers.size(); i++)
	{
		if (m_buffers[i].first >= idx)
		{
			m_buffers.insert(i, Pair(idx, buffer));
			inserted = true;
			break;
		}
	}

	if (!inserted) {
		m_buffers.emplaceBack(idx, buffer);
	}

	m_descriptorBuilder.bindBuffer(
		idx,
		&buffer->getDescriptor(),
		buffer->getDescriptorType(),
		m_stage,
		1
	);
}
