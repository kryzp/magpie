#include "descriptor.h"

#include "core/common.h"

#include "math/calc.h"

#include "core.h"
#include "image_view.h"
#include "sampler.h"
#include "gpu_buffer.h"

using namespace mgp;

void DescriptorPoolStatic::init(const VulkanCore *core, uint32_t maxSets, VkDescriptorPoolCreateFlags flags, const std::vector<DescriptorPoolSize> &sizes)
{
	m_core = core;

	std::vector<VkDescriptorPoolSize> poolSizes(sizes.size());

	for (int i = 0; i < sizes.size(); i++)
	{
		poolSizes[i].type = sizes[i].type;
		poolSizes[i].descriptorCount = sizes[i].max;
	}

	VkDescriptorPoolCreateInfo poolCreateInfo = {};
	poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolCreateInfo.flags = flags;
	poolCreateInfo.maxSets = maxSets;
	poolCreateInfo.poolSizeCount = poolSizes.size();
	poolCreateInfo.pPoolSizes = poolSizes.data();

	MGP_VK_CHECK(
		vkCreateDescriptorPool(m_core->getLogicalDevice(), &poolCreateInfo, nullptr, &m_pool),
		"Failed to create static descriptor pool"
	);
}

void DescriptorPoolStatic::cleanUp()
{
	vkDestroyDescriptorPool(m_core->getLogicalDevice(), m_pool, nullptr);
	m_pool = VK_NULL_HANDLE;
}

void DescriptorPoolStatic::clear()
{
	vkResetDescriptorPool(m_core->getLogicalDevice(), m_pool, 0);
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

	MGP_VK_CHECK(
		vkAllocateDescriptorSets(m_core->getLogicalDevice(), &allocInfo, &set),
		"Failed to allocate descriptor set"
	);

	return set;
}

VkDescriptorPool DescriptorPoolStatic::getPool() const
{
	return m_pool;
}

// ack: mostly just vkguide, thank you!
void DescriptorPoolDynamic::init(const VulkanCore *core, uint32_t initialSets, const std::vector<DescriptorPoolSize> &sizes)
{
	m_sizes = sizes;
	m_core = core;

	VkDescriptorPool newPool = createNewPool(initialSets, sizes);

	m_freePools.push_back(newPool);
	m_setsPerPool = initialSets * GROWTH_FACTOR;
}

void DescriptorPoolDynamic::cleanUp()
{
	for (auto p : m_freePools)
	{
		vkDestroyDescriptorPool(m_core->getLogicalDevice(), p, nullptr);
	}

	for (auto p : m_usedPools)
	{
		vkDestroyDescriptorPool(m_core->getLogicalDevice(), p, nullptr);
	}

	m_freePools.clear();
	m_usedPools.clear();
}

void DescriptorPoolDynamic::clear()
{
	for (auto p : m_freePools)
	{
		vkResetDescriptorPool(m_core->getLogicalDevice(), p, 0);
	}

	for (auto p : m_usedPools)
	{
		vkResetDescriptorPool(m_core->getLogicalDevice(), p, 0);
		m_freePools.push_back(p);
	}

	m_usedPools.clear();
}

VkDescriptorSet DescriptorPoolDynamic::allocate(const std::vector<VkDescriptorSetLayout> &layouts, void *pNext)
{
	VkDescriptorPool allocatedPool = fetchPool();

	VkDescriptorSet ret = VK_NULL_HANDLE;

	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.pNext = pNext;
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = allocatedPool;
	allocInfo.descriptorSetCount = layouts.size();
	allocInfo.pSetLayouts = layouts.data();

	VkResult result = vkAllocateDescriptorSets(m_core->getLogicalDevice(), &allocInfo, &ret);

	if (result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_FRAGMENTED_POOL) {

		m_usedPools.push_back(allocatedPool);

		VkDescriptorPool allocatedPool = fetchPool();
		allocInfo.descriptorPool = allocatedPool;

		MGP_VK_CHECK(
			vkAllocateDescriptorSets(m_core->getLogicalDevice(), &allocInfo, &ret),
			"Failed to allocate descriptor sets"
		);
	}

	m_freePools.push_back(allocatedPool);

	return ret;
}

VkDescriptorPool DescriptorPoolDynamic::fetchPool()
{
	VkDescriptorPool newPool = VK_NULL_HANDLE;

	if (m_freePools.size() != 0)
	{
		newPool = m_freePools.back();
		m_freePools.pop_back();
	}
	else
	{
		newPool = createNewPool(m_setsPerPool, m_sizes);
		m_setsPerPool = Calc<uint32_t>::min(m_setsPerPool * GROWTH_FACTOR, 4092);
	}   

	return newPool;
}

VkDescriptorPool DescriptorPoolDynamic::createNewPool(uint32_t maxSets, const std::vector<DescriptorPoolSize> &sizes)
{
	std::vector<VkDescriptorPoolSize> poolSizes(sizes.size());

	for (int i = 0; i < sizes.size(); i++)
	{
		poolSizes[i].type = sizes[i].type;
		poolSizes[i].descriptorCount = sizes[i].max;
	}

	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = 0;
	pool_info.maxSets = maxSets;
	pool_info.poolSizeCount = (uint32_t)poolSizes.size();
	pool_info.pPoolSizes = poolSizes.data();

	VkDescriptorPool newPool = {};

	vkCreateDescriptorPool(m_core->getLogicalDevice(), &pool_info, nullptr, &newPool);

	return newPool;
}

void DescriptorLayoutCache::init(const VulkanCore *core)
{
	m_core = core;
}

void DescriptorLayoutCache::dispose()
{
	for (auto &[id, layout] : m_layoutCache)
	{
		vkDestroyDescriptorSetLayout(m_core->getLogicalDevice(), layout, nullptr);
	}

	m_layoutCache.clear();
}

VkDescriptorSetLayout DescriptorLayoutCache::createLayout(const VkDescriptorSetLayoutCreateInfo &layoutCreateInfo)
{
	uint64_t hash = 0;

	hash::combine(&hash, &layoutCreateInfo.bindingCount);

	for (int i = 0; i < layoutCreateInfo.bindingCount; i++)
	{
		hash::combine(&hash, &layoutCreateInfo.pBindings[i]);
	}

	if (m_layoutCache.contains(hash))
		return m_layoutCache[hash];

	VkDescriptorSetLayout layout = {};

	MGP_VK_CHECK(
		vkCreateDescriptorSetLayout(m_core->getLogicalDevice(), &layoutCreateInfo, nullptr, &layout),
		"Failed to create descriptor set layout"
	);

	m_layoutCache.insert({hash, layout});

	return layout;
}

VkDescriptorSetLayout DescriptorLayoutBuilder::build(VulkanCore *core, VkShaderStageFlags shaderStages, void *pNext, VkDescriptorSetLayoutCreateFlags flags)
{
	for (auto &binding : m_bindings)
	{
		binding.stageFlags |= shaderStages;
	}

	VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {};
	layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutCreateInfo.pNext = pNext;
	layoutCreateInfo.flags = flags;
	layoutCreateInfo.bindingCount = m_bindings.size();
	layoutCreateInfo.pBindings = m_bindings.data();

	return core->getDescriptorLayoutCache().createLayout(layoutCreateInfo);
}

DescriptorLayoutBuilder &DescriptorLayoutBuilder::bind(uint32_t bindingIndex, VkDescriptorType type, uint32_t descriptorCount)
{
	VkDescriptorSetLayoutBinding binding = {};
	binding.binding = bindingIndex;
	binding.descriptorType = type;
	binding.descriptorCount = descriptorCount;
	binding.stageFlags = 0;
	binding.pImmutableSamplers = nullptr;

	m_bindings.push_back(binding);

	return *this;
}

void DescriptorWriter::clear()
{
	m_writes.clear();
}

void DescriptorWriter::updateSet(const VulkanCore *core, const VkDescriptorSet &set)
{
	for (auto &write : m_writes)
	{
		write.dstSet = set;
	}

	vkUpdateDescriptorSets(
		core->getLogicalDevice(),
		m_writes.size(), m_writes.data(),
		0, nullptr
	);
}

DescriptorWriter &DescriptorWriter::writeBuffer(uint32_t bindingIndex, VkDescriptorType type, const VkDescriptorBufferInfo &info, uint32_t arrayIndex)
{
	m_bufferInfos[m_nBufferInfos] = info;

	VkWriteDescriptorSet write = {};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.dstBinding = bindingIndex;
	write.dstSet = VK_NULL_HANDLE; // set on updateSet()
	write.dstArrayElement = arrayIndex;
	write.descriptorCount = 1;
	write.descriptorType = type;
	write.pBufferInfo = &m_bufferInfos[m_nBufferInfos];

	m_writes.push_back(write);

	m_nBufferInfos++;

	return *this;
}

DescriptorWriter &DescriptorWriter::writeImage(uint32_t bindingIndex, VkDescriptorType type, const VkDescriptorImageInfo &info, uint32_t arrayIndex)
{
	m_imageInfos[m_nImageInfos] = info;

	VkWriteDescriptorSet write = {};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.dstBinding = bindingIndex;
	write.dstSet = VK_NULL_HANDLE; // set on updateSet()
	write.dstArrayElement = arrayIndex;
	write.descriptorCount = 1;
	write.descriptorType = type;
	write.pImageInfo = &m_imageInfos[m_nImageInfos];

	m_writes.push_back(write);

	m_nImageInfos++;

	return *this;
}

DescriptorWriter &DescriptorWriter::writeCombinedImage(uint32_t bindingIndex, const ImageView &view, const Sampler &sampler, uint32_t arrayIndex)
{
	VkDescriptorImageInfo info = {};
	info.imageView = view.getHandle();
	info.imageLayout = view.getImage()->isDepth() ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	info.sampler = sampler.getHandle();

	return writeImage(bindingIndex, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, info, arrayIndex);
}

DescriptorWriter &DescriptorWriter::writeSampledImage(uint32_t bindingIndex, const ImageView &view, uint32_t arrayIndex)
{
	VkDescriptorImageInfo info = {};
	info.imageView = view.getHandle();
	info.imageLayout = view.getImage()->isDepth() ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	info.sampler = VK_NULL_HANDLE;

	return writeImage(bindingIndex, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, info, arrayIndex);
}

DescriptorWriter &DescriptorWriter::writeStorageImage(uint32_t bindingIndex, const ImageView &view, uint32_t arrayIndex)
{
	VkDescriptorImageInfo info = {};
	info.imageView = view.getHandle();
	info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	info.sampler = VK_NULL_HANDLE;

	return writeImage(bindingIndex, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, info, arrayIndex);
}

DescriptorWriter &DescriptorWriter::writeSampler(uint32_t bindingIndex, const Sampler &sampler, uint32_t arrayIndex)
{
	VkDescriptorImageInfo info = {};
	info.imageView = VK_NULL_HANDLE;
	info.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	info.sampler = sampler.getHandle();

	return writeImage(bindingIndex, VK_DESCRIPTOR_TYPE_SAMPLER, info, arrayIndex);
}
