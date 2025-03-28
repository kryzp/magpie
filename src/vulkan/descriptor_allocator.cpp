#include "descriptor_allocator.h"

#include "core.h"

#include "math/calc.h"

// ack: mostly just vkguide, thank you!

using namespace llt;

void DescriptorPoolStatic::init(uint32_t maxSets, VkDescriptorPoolCreateFlags flags, const Vector<DescriptorPoolSizeRatio> &sizes)
{
	Vector<VkDescriptorPoolSize> poolSizes;

	for (auto &ratio : sizes)
	{
		poolSizes.emplaceBack(
			ratio.first,
			(uint32_t)(ratio.second * maxSets)
		);
	}

	VkDescriptorPoolCreateInfo poolCreateInfo = {};
	poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolCreateInfo.flags = flags;
	poolCreateInfo.maxSets = maxSets;
	poolCreateInfo.poolSizeCount = poolSizes.size();
	poolCreateInfo.pPoolSizes = poolSizes.data();

	LLT_VK_CHECK(
		vkCreateDescriptorPool(g_vkCore->m_device, &poolCreateInfo, nullptr, &m_pool),
		"Failed to create static descriptor pool"
	);
}

void DescriptorPoolStatic::cleanUp()
{
	if (m_pool == VK_NULL_HANDLE)
		return;

	vkDestroyDescriptorPool(g_vkCore->m_device, m_pool, nullptr);
	m_pool = VK_NULL_HANDLE;
}

void DescriptorPoolStatic::clear()
{
	vkResetDescriptorPool(g_vkCore->m_device, m_pool, 0);
}

VkDescriptorSet DescriptorPoolStatic::allocate(VkDescriptorSetLayout layout)
{
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.pNext = nullptr;
	allocInfo.descriptorPool = m_pool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &layout;

	VkDescriptorSet set = VK_NULL_HANDLE;

	LLT_VK_CHECK(
		vkAllocateDescriptorSets(g_vkCore->m_device, &allocInfo, &set),
		"Failed to allocate descriptor set"
	);

	return set;
}

VkDescriptorPool DescriptorPoolStatic::getPool() const
{
	return m_pool;
}

// ===================================================================================== //

void DescriptorPoolDynamic::init(uint32_t initialSets, const Vector<DescriptorPoolSizeRatio> &sizes)
{
	m_sizes = sizes;

	VkDescriptorPool newPool = createNewPool(initialSets, sizes);

	m_freePools.pushBack(newPool);
	m_setsPerPool = initialSets * GROWTH_FACTOR;
}

void DescriptorPoolDynamic::cleanUp()
{
	for (auto p : m_freePools)
	{
		vkDestroyDescriptorPool(g_vkCore->m_device, p, nullptr);
	}

	for (auto p : m_usedPools)
	{
		vkDestroyDescriptorPool(g_vkCore->m_device, p, nullptr);
	}

	m_freePools.clear();
	m_usedPools.clear();
}

void DescriptorPoolDynamic::clear()
{
	for (auto p : m_freePools)
	{
		vkResetDescriptorPool(g_vkCore->m_device, p, 0);
	}

	for (auto p : m_usedPools)
	{
		vkResetDescriptorPool(g_vkCore->m_device, p, 0);
		m_freePools.pushBack(p);
	}
	
	m_usedPools.clear();
}

VkDescriptorSet DescriptorPoolDynamic::allocate(const Vector<VkDescriptorSetLayout> &layouts, void *pNext)
{
	VkDescriptorPool allocatedPool = fetchPool();

	VkDescriptorSet ret = VK_NULL_HANDLE;

	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.pNext = pNext;
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = allocatedPool;
	allocInfo.descriptorSetCount = layouts.size();
	allocInfo.pSetLayouts = layouts.data();

	VkResult result = vkAllocateDescriptorSets(g_vkCore->m_device, &allocInfo, &ret);

	if (result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_FRAGMENTED_POOL) {

		m_usedPools.pushBack(allocatedPool);

		VkDescriptorPool allocatedPool = fetchPool();
		allocInfo.descriptorPool = allocatedPool;

		LLT_VK_CHECK(
			vkAllocateDescriptorSets(g_vkCore->m_device, &allocInfo, &ret),
			"Failed to allocate descriptor sets"
		);
	}

	m_freePools.pushBack(allocatedPool);

	return ret;
}

VkDescriptorPool DescriptorPoolDynamic::fetchPool()
{
	VkDescriptorPool newPool = VK_NULL_HANDLE;

	if (m_freePools.size() != 0)
	{
		newPool = m_freePools.popBack();
	}
	else
	{
		newPool = createNewPool(m_setsPerPool, m_sizes);
		m_setsPerPool = Calc<uint32_t>::min(m_setsPerPool * GROWTH_FACTOR, 4092);
	}   

	return newPool;
}

VkDescriptorPool DescriptorPoolDynamic::createNewPool(uint32_t setCount, const Vector<DescriptorPoolSizeRatio> &sizes)
{
	Vector<VkDescriptorPoolSize> poolSizes;

	for (auto &ratio : sizes)
	{
		poolSizes.emplaceBack(
			ratio.first,
			(uint32_t)(ratio.second * setCount)
		);
	}

	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = 0;
	pool_info.maxSets = setCount;
	pool_info.poolSizeCount = (uint32_t)poolSizes.size();
	pool_info.pPoolSizes = poolSizes.data();

	VkDescriptorPool newPool = {};

	vkCreateDescriptorPool(g_vkCore->m_device, &pool_info, nullptr, &newPool);

	return newPool;
}
