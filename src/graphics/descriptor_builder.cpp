#include "descriptor_builder.h"
#include "descriptor_layout_cache.h"
#include "backend.h"

using namespace llt;

VkDescriptorSetLayout DescriptorLayoutBuilder::build(VkShaderStageFlags shaderStages, DescriptorLayoutCache* cache, void* pNext, VkDescriptorSetLayoutCreateFlags flags)
{
	for (auto& binding : m_bindings)
	{
		binding.stageFlags |= shaderStages;
	}

	VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {};
	layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutCreateInfo.pNext = pNext;
	layoutCreateInfo.flags = flags;
	layoutCreateInfo.bindingCount = m_bindings.size();
	layoutCreateInfo.pBindings = m_bindings.data();

	VkDescriptorSetLayout set = VK_NULL_HANDLE;
	
	if (cache)
	{
		set = cache->createLayout(layoutCreateInfo);
	}
	else
	{
		LLT_VK_CHECK(
			vkCreateDescriptorSetLayout(g_vulkanBackend->device, &layoutCreateInfo, nullptr, &set),
			"Failed to create descriptor set layout"
		);
	}

	return set;
}

void DescriptorLayoutBuilder::bind(uint32_t idx, VkDescriptorType type)
{
	VkDescriptorSetLayoutBinding binding = {};
	binding.binding = idx;
	binding.descriptorType = type;
	binding.descriptorCount = 1;
	binding.stageFlags = 0;
	binding.pImmutableSamplers = nullptr;

	m_bindings.pushBack(binding);
}

void DescriptorLayoutBuilder::clear()
{
	m_bindings.clear();
}

// //

void DescriptorWriter::clear()
{
	m_writes.clear();

	m_bufferInfos.clear();
	m_imageInfos.clear();
}

void DescriptorWriter::updateSet(VkDescriptorSet set)
{
	for (auto& write : m_writes)
	{
		write.dstSet = set;
	}

	vkUpdateDescriptorSets(g_vulkanBackend->device, m_writes.size(), m_writes.data(), 0, nullptr);
}

void DescriptorWriter::writeBuffer(uint32_t idx, VkDescriptorType type, const VkDescriptorBufferInfo& info)
{
	m_bufferInfos.pushBack(info);

	VkWriteDescriptorSet write = {};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.dstBinding = idx;
	write.dstSet = VK_NULL_HANDLE;
	write.descriptorCount = 1;
	write.descriptorType = type;
	write.pBufferInfo = &m_bufferInfos.back();

	m_writes.pushBack(write);
}

void DescriptorWriter::writeBuffer(uint32_t idx, VkDescriptorType type, VkBuffer buffer, uint64_t size, uint64_t offset)
{
	VkDescriptorBufferInfo info = {};
	info.buffer = buffer;
	info.offset = offset;
	info.range = size;

	writeBuffer(idx, type, info);
}

void DescriptorWriter::writeImage(uint32_t idx, VkDescriptorType type, const VkDescriptorImageInfo& info)
{
	m_imageInfos.pushBack(info);

	VkWriteDescriptorSet write = {};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.dstBinding = idx;
	write.dstSet = VK_NULL_HANDLE;
	write.descriptorCount = 1;
	write.descriptorType = type;
	write.pImageInfo = &m_imageInfos.back();

	m_writes.pushBack(write);
}

void DescriptorWriter::writeImage(uint32_t idx, VkDescriptorType type, VkImageView image, VkSampler sampler, VkImageLayout layout)
{
	VkDescriptorImageInfo info = {};
	info.sampler = sampler;
	info.imageView = image;
	info.imageLayout = layout;

	writeImage(idx, type, info);
}

/*
uint64_t DescriptorBuilder::getHash() const
{
	uint64_t result = 0;

	for (auto& binding : m_bindings) {
		hash::combine(&result, &binding);
	}

	for (auto& binding : m_writes) {
		hash::combine(&result, &binding);
	}

	return result;
}

Vector<uint32_t> DescriptorBuilder::getDynamicOffsets() const
{
	Vector<uint32_t> result;

	for (auto& elem : m_dynamicOffsets) {
		result.pushBack(elem.second);
	}

	return result;
}

DescriptorBuilder DescriptorBuilder::begin(DescriptorCache& cache, DescriptorPoolDynamic& allocator)
{
	DescriptorBuilder builder;
	builder.m_cache = &cache;
	builder.m_poolAllocator = &allocator;

	return builder;
}

VkDescriptorSet DescriptorBuilder::buildGivenLayout(const VkDescriptorSetLayout& layout, uint32_t count)
{
	uint64_t hash = 0;

	for (auto& write : m_writes)
	{
		if (write.pImageInfo)
			hash::combine(&hash, write.pImageInfo);
	}

	bool needsUpdating = false;
	VkDescriptorSet set = m_cache->createSet(layout, count, hash, m_poolAllocator, &needsUpdating);

	for (auto& write : m_writes) {
		write.dstSet = set;
	}

	if (needsUpdating) {
		vkUpdateDescriptorSets(
			g_vulkanBackend->device,
			m_writes.size(),
			m_writes.data(),
			0, nullptr
		);
	}

	return set;
}

VkDescriptorSet DescriptorBuilder::build(uint32_t count, VkDescriptorSetLayoutCreateFlags layoutFlags)
{
	VkDescriptorSetLayout layout = buildLayout(layoutFlags);
	return buildGivenLayout(layout, count);
}

VkDescriptorSetLayout DescriptorBuilder::buildLayout(VkDescriptorSetLayoutCreateFlags flags)
{
	VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlags = {};
	bindingFlags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
	bindingFlags.bindingCount = m_flags.size();
	bindingFlags.pBindingFlags = m_flags.data();
	bindingFlags.pNext = nullptr;

	VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {};
	layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutCreateInfo.bindingCount = m_bindings.size();
	layoutCreateInfo.pBindings = m_bindings.data();
	layoutCreateInfo.flags = flags;
	layoutCreateInfo.pNext = &bindingFlags;

	return m_cache->createLayout(layoutCreateInfo);
}

DescriptorBuilder& DescriptorBuilder::bindBuffer(
	uint32_t idx,
	const VkDescriptorBufferInfo* info,
	VkDescriptorType type,
	VkShaderStageFlags stageFlags,
	int count,
	uint32_t dynamicOffset,
	VkDescriptorBindingFlags flags
)
{
	bool inserted = false;

	for (int i = 0; i < m_dynamicOffsets.size(); i++)
	{
		if (m_dynamicOffsets[i].first >= idx)
		{
			m_dynamicOffsets.insert(i, Pair(idx, dynamicOffset));
			inserted = true;
			break;
		}
	}

	if (!inserted) {
		m_dynamicOffsets.emplaceBack(idx, dynamicOffset);
	}

	m_flags.pushBack(flags);

	VkDescriptorSetLayoutBinding binding = {};
	binding.binding = idx;
	binding.descriptorType = type;
	binding.descriptorCount = count;
	binding.stageFlags = stageFlags;
	binding.pImmutableSamplers = nullptr;
	m_bindings.pushBack(binding);

	VkWriteDescriptorSet write = {};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.dstBinding = idx;
	write.dstArrayElement = 0;
	write.descriptorType = type;
	write.descriptorCount = 1;
	write.pBufferInfo = info;
	m_writes.pushBack(write);

	return *this;
}

DescriptorBuilder& DescriptorBuilder::bindImage(
	uint32_t idx,
	const VkDescriptorImageInfo* info,
	VkDescriptorType type,
	VkShaderStageFlags stageFlags,
	int count,
	VkDescriptorBindingFlags flags
)
{
	m_flags.pushBack(flags);

	VkDescriptorSetLayoutBinding binding = {};
	binding.binding = idx;
	binding.descriptorType = type;
	binding.descriptorCount = count;
	binding.stageFlags = stageFlags;
	binding.pImmutableSamplers = nullptr;
	m_bindings.pushBack(binding);

	VkWriteDescriptorSet write = {};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.dstBinding = idx;
	write.dstArrayElement = 0;
	write.descriptorType = type;
	write.descriptorCount = 1;
	write.pImageInfo = info;
	m_writes.pushBack(write);

	return *this;
}
*/
