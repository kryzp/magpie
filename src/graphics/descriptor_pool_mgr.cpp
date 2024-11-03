#include "descriptor_pool_mgr.h"
#include "backend.h"

using namespace llt;

DescriptorPoolMgr::DescriptorPoolMgr()
	: m_freePools()
	, m_usedPools()
	, m_currentPool(VK_NULL_HANDLE)
{
}

DescriptorPoolMgr::~DescriptorPoolMgr()
{
}

void DescriptorPoolMgr::init()
{
	initSizes();
}

void DescriptorPoolMgr::initSizes()
{
	m_sizes.emplaceBack(VK_DESCRIPTOR_TYPE_SAMPLER, 					0.5f);
	m_sizes.emplaceBack(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 	4.0f);
	m_sizes.emplaceBack(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 				4.0f);
	m_sizes.emplaceBack(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 				1.0f);
	m_sizes.emplaceBack(VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 		1.0f);
	m_sizes.emplaceBack(VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 		1.0f);
	m_sizes.emplaceBack(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 			2.0f);
	m_sizes.emplaceBack(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 			2.0f);
	m_sizes.emplaceBack(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 	1.0f);
	m_sizes.emplaceBack(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 	1.0f);
	m_sizes.emplaceBack(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 			0.5f);
}

void DescriptorPoolMgr::cleanUp()
{
	resetPools();

	for (auto& pool : m_freePools) {
		vkDestroyDescriptorPool(g_vulkanBackend->device, pool, nullptr);
	}

	m_freePools.clear();

	for (auto& pool : m_usedPools) {
		vkDestroyDescriptorPool(g_vulkanBackend->device, pool, nullptr);
	}

	m_usedPools.clear();
}

void DescriptorPoolMgr::resetPools()
{
	for (auto& pool : m_usedPools) {
		vkResetDescriptorPool(g_vulkanBackend->device, pool, 0);
		m_freePools.pushBack(pool);
	}

	m_usedPools.clear();
	m_currentPool = VK_NULL_HANDLE;
}

VkDescriptorSet DescriptorPoolMgr::allocateDescriptorSet(const VkDescriptorSetLayout& layout)
{
	// if our current pool is null then get a new one and send it to the used pools
	if (m_currentPool == VK_NULL_HANDLE) {
		m_currentPool = fetchPool();
		m_usedPools.pushBack(m_currentPool);
	}

	VkDescriptorSet ret = {};

	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_currentPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &layout;

	VkResult result = vkAllocateDescriptorSets(g_vulkanBackend->device, &allocInfo, &ret);
	bool reallocate_memory = false;

	if (result == VK_SUCCESS) {
		LLT_LOG("[VULKAN:DESCRIPTORPOOL] Created descriptor sets successfully!");
		return ret;
	} else if (result == VK_ERROR_FRAGMENTED_POOL || result == VK_ERROR_OUT_OF_POOL_MEMORY) {
		LLT_LOG("[VULKAN:DESCRIPTORPOOL] Failed to allocate descriptor sets initially, reallocating memory...");
		reallocate_memory = true;
	} else {
		LLT_ERROR("[VULKAN:DESCRIPTORPOOL|DEBUG] Encountered unknown return result from vkAllocateDescriptorSets: %d", result);
	}

	// if we find that we are fragmented then try to just reallocate
	if (reallocate_memory)
	{
		// get a new current pool
		m_currentPool = fetchPool();
		m_usedPools.pushBack(m_currentPool);

		// allocate again
		result = vkAllocateDescriptorSets(g_vulkanBackend->device, &allocInfo, &ret);

		// if we still fail then throw an error
		if (result != VK_SUCCESS) {
			LLT_ERROR("[VULKAN:DESCRIPTORPOOL|DEBUG] Failed to create descriptor sets even after reallocation, allocation result: %d", result);
		} else {
			LLT_LOG("[VULKAN:DESCRIPTORPOOL] Created descriptor sets successfully after reallocating!");
		}
	}

	return ret;
}

VkDescriptorPool DescriptorPoolMgr::createNewPool(uint32_t count)
{
	Vector<VkDescriptorPoolSize> poolSizes;

	for (auto& size : m_sizes) {
		poolSizes.pushBack({ size.first, (uint32_t)(size.second * count) });
	}

	VkDescriptorPoolCreateInfo poolCreateInfo = {};
	poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolCreateInfo.flags = 0;
	poolCreateInfo.poolSizeCount = poolSizes.size();
	poolCreateInfo.pPoolSizes = poolSizes.data();
	poolCreateInfo.maxSets = count;

	VkDescriptorPool pool = {};

	if (VkResult result = vkCreateDescriptorPool(g_vulkanBackend->device, &poolCreateInfo, nullptr, &pool); result != VK_SUCCESS) {
		LLT_ERROR("[VULKAN:DESCRIPTORPOOL|DEBUG] Failed to create descriptor pool: %d", result);
	}

	LLT_LOG("[VULKAN:DESCRIPTORPOOL] Created new descriptor pool!");

	return pool;
}

VkDescriptorPool DescriptorPoolMgr::fetchPool()
{
	if (m_freePools.size() > 0) {
		VkDescriptorPool pool = m_freePools.back();
		m_freePools.popBack();
		return pool;
	} else {
		return createNewPool(64 * mgc::FRAMES_IN_FLIGHT);
	}
}
