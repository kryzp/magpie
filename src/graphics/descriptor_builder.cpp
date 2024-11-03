#include "descriptor_builder.h"
#include "backend.h"

using namespace llt;

void DescriptorBuilder::reset(DescriptorPoolMgr* mgr, DescriptorCache* cache)
{
	m_mgr = mgr;
	m_cache = cache;

	m_bindings.clear();
	m_writes.clear();
}

uint64_t DescriptorBuilder::hash() const
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

void DescriptorBuilder::build(VkDescriptorSet& set, VkDescriptorSetLayout& layout, uint64_t hash)
{
	buildLayout(layout);

	bool needsUpdating = false;
	set = m_cache->createSet(m_mgr, layout, hash, &needsUpdating);

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
}

void DescriptorBuilder::buildLayout(VkDescriptorSetLayout& layout)
{
	VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {};
	layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutCreateInfo.bindingCount = m_bindings.size();
	layoutCreateInfo.pBindings = m_bindings.data();

	layout = m_cache->createLayout(layoutCreateInfo);
}

DescriptorBuilder& DescriptorBuilder::bindBuffer(
	uint32_t idx,
	const VkDescriptorBufferInfo* info,
	VkDescriptorType type,
	VkShaderStageFlags stageFlags
)
{
	VkDescriptorSetLayoutBinding binding = {};
	binding.binding = idx;
	binding.descriptorType = type;
	binding.descriptorCount = 1;
	binding.stageFlags = stageFlags;
	binding.pImmutableSamplers = nullptr;
	m_bindings.pushBack(binding);

	VkWriteDescriptorSet write = {};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.dstBinding = idx;
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
	VkShaderStageFlags stageFlags
)
{
	VkDescriptorSetLayoutBinding binding = {};
	binding.binding = idx;
	binding.descriptorType = type;
	binding.descriptorCount = 1;
	binding.stageFlags = stageFlags;
	binding.pImmutableSamplers = nullptr;
	m_bindings.pushBack(binding);

	VkWriteDescriptorSet write = {};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.dstBinding = idx;
	write.descriptorType = type;
	write.descriptorCount = 1;
	write.pImageInfo = info;
	m_writes.pushBack(write);

	return *this;
}
