#include "descriptor_cache.h"
#include "descriptor_pool_mgr.h"
#include "backend.h"

using namespace llt;

DescriptorCache::DescriptorCache()
	: m_layoutCache()
{
}

DescriptorCache::~DescriptorCache()
{
}

void DescriptorCache::cleanUp()
{
	clearSetCache();

	for (auto& [id, layout] : m_layoutCache) {
		vkDestroyDescriptorSetLayout(g_vulkanBackend->device, layout, nullptr);
	}

	m_layoutCache.clear();
}

void DescriptorCache::clearSetCache()
{
	m_descriptorCache.clear();
}

VkDescriptorSet DescriptorCache::createSet(const VkDescriptorSetLayout& layout, uint64_t hash, bool* wasNotCached)
{
	if (m_descriptorCache.contains(hash)) {
		if (wasNotCached) {
			(*wasNotCached) = false;
		}
		return m_descriptorCache[hash];
	} else {
		if (wasNotCached) {
			(*wasNotCached) = true;
		}
	}

	VkDescriptorSet set = g_vulkanBackend->descriptorPoolManager.allocateDescriptorSet(layout);
	m_descriptorCache.insert(hash, set);

	return set;
}

VkDescriptorSetLayout DescriptorCache::createLayout(const VkDescriptorSetLayoutCreateInfo& layoutCreateInfo)
{
	// create a unique hash for the layout create info
	uint64_t createdDescriptorHash = 0;

	hash::combine(&createdDescriptorHash, &layoutCreateInfo.bindingCount);

	for (int i = 0; i < layoutCreateInfo.bindingCount; i++) {
		hash::combine(&createdDescriptorHash, &layoutCreateInfo.pBindings[i]);
	}

	// check if its already cached, if it is, return it.
	if (m_layoutCache.contains(createdDescriptorHash)) {
		return m_layoutCache[createdDescriptorHash];
	}

	// make a new one
	VkDescriptorSetLayout createdDescriptor = {};

	if (VkResult result = vkCreateDescriptorSetLayout(g_vulkanBackend->device, &layoutCreateInfo, nullptr, &createdDescriptor); result != VK_SUCCESS) {
		LLT_ERROR("[VULKAN:DESCRIPTORBUILDER|DEBUG] Failed to create descriptor set layout: %d", result);
	}

	// cache it
	m_layoutCache.insert(
		createdDescriptorHash,
		createdDescriptor
	);

	return createdDescriptor;
}
