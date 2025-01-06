#include "descriptor_builder.h"
#include "backend.h"

using namespace llt;

void DescriptorBuilder::clear()
{
	m_flags.clear();
	m_bindings.clear();
	m_writes.clear();
}

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

void DescriptorBuilder::build(VkDescriptorSet& set, const VkDescriptorSetLayout& layout, uint32_t count, uint64_t hash)
{
	bool needsUpdating = false;
	set = g_vulkanBackend->descriptorCache.createSet(layout, count, hash, &needsUpdating);

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

void DescriptorBuilder::buildLayout(VkDescriptorSetLayout& layout, VkDescriptorSetLayoutCreateFlags flags)
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

	layout = g_vulkanBackend->descriptorCache.createLayout(layoutCreateInfo);
}

void DescriptorBuilder::bindBuffer(
	uint32_t idx,
	const VkDescriptorBufferInfo* info,
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
	write.pBufferInfo = info;
	m_writes.pushBack(write);
}

void DescriptorBuilder::bindImage(
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
}
