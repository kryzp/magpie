#include "descriptor_layout_cache.h"
#include "backend.h"

using namespace llt;

void DescriptorLayoutCache::cleanUp()
{
	for (auto &[id, layout] : m_layoutCache) {
		vkDestroyDescriptorSetLayout(g_vulkanBackend->m_device, layout, nullptr);
	}

	m_layoutCache.clear();
}

VkDescriptorSetLayout DescriptorLayoutCache::createLayout(const VkDescriptorSetLayoutCreateInfo &layoutCreateInfo)
{
	uint64_t hash = 0;

	hash::combine(&hash, &layoutCreateInfo.bindingCount);

	for (int i = 0; i < layoutCreateInfo.bindingCount; i++) {
		hash::combine(&hash, &layoutCreateInfo.pBindings[i]);
	}

	if (m_layoutCache.contains(hash)) {
		return m_layoutCache[hash];
	}

	VkDescriptorSetLayout layout = {};

	LLT_VK_CHECK(
		vkCreateDescriptorSetLayout(g_vulkanBackend->m_device, &layoutCreateInfo, nullptr, &layout),
		"Failed to create descriptor set layout"
	);

	m_layoutCache.insert(hash, layout);

	return layout;
}
