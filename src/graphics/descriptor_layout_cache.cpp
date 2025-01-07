#include "descriptor_layout_cache.h"
#include "backend.h"

using namespace llt;

void DescriptorLayoutCache::cleanUp()
{
	for (auto& [id, layout] : m_layoutCache)
	{
		vkDestroyDescriptorSetLayout(g_vulkanBackend->device, layout, nullptr);
	}

	m_layoutCache.clear();
}

VkDescriptorSetLayout DescriptorLayoutCache::createLayout(const VkDescriptorSetLayoutCreateInfo& layoutCreateInfo)
{
	uint64_t createdDescriptorHash = 0;

	hash::combine(&createdDescriptorHash, &layoutCreateInfo.bindingCount);

	for (int i = 0; i < layoutCreateInfo.bindingCount; i++) {
		hash::combine(&createdDescriptorHash, &layoutCreateInfo.pBindings[i]);
	}

	if (m_layoutCache.contains(createdDescriptorHash)) {
		return m_layoutCache[createdDescriptorHash];
	}

	VkDescriptorSetLayout createdDescriptor = {};

	LLT_VK_CHECK(
		vkCreateDescriptorSetLayout(g_vulkanBackend->device, &layoutCreateInfo, nullptr, &createdDescriptor),
		"Failed to create descriptor set layout"
	);

	m_layoutCache.insert(
		createdDescriptorHash,
		createdDescriptor
	);

	return createdDescriptor;
}
