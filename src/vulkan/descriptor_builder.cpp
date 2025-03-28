#include "descriptor_builder.h"
#include "descriptor_layout_cache.h"
#include "core.h"

using namespace llt;

VkDescriptorSetLayout DescriptorLayoutBuilder::build(VkShaderStageFlags shaderStages, void *pNext, VkDescriptorSetLayoutCreateFlags flags)
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

	return g_vkCore->getDescriptorLayoutCache().createLayout(layoutCreateInfo);
}

DescriptorLayoutBuilder &DescriptorLayoutBuilder::bind(uint32_t bindingIndex, VkDescriptorType type, uint32_t descriptorCount)
{
	VkDescriptorSetLayoutBinding binding = {};
	binding.binding = bindingIndex;
	binding.descriptorType = type;
	binding.descriptorCount = descriptorCount;
	binding.stageFlags = 0;
	binding.pImmutableSamplers = nullptr;

	m_bindings.pushBack(binding);

	return *this;
}

// ---

void DescriptorWriter::clear()
{
	m_writes.clear();
}

void DescriptorWriter::updateSet(const VkDescriptorSet &set)
{
	for (auto &write : m_writes)
	{
		write.dstSet = set;
	}

	vkUpdateDescriptorSets(
		g_vkCore->m_device,
		m_writes.size(), m_writes.data(),
		0, nullptr
	);
}

DescriptorWriter &DescriptorWriter::writeBuffer(uint32_t bindingIndex, VkDescriptorType type, const VkDescriptorBufferInfo &info, uint32_t dstArrayElement)
{
	m_bufferInfos[m_nBufferInfos] = info;

	VkWriteDescriptorSet write = {};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.dstBinding = bindingIndex;
	write.dstSet = VK_NULL_HANDLE; // set on updateSet()
	write.dstArrayElement = dstArrayElement;
	write.descriptorCount = 1;
	write.descriptorType = type;
	write.pBufferInfo = &m_bufferInfos[m_nBufferInfos];

	m_writes.pushBack(write);

	m_nBufferInfos++;

	return *this;
}

DescriptorWriter &DescriptorWriter::writeBuffer(uint32_t bindingIndex, VkDescriptorType type, VkBuffer buffer, uint64_t size, uint64_t offset, uint32_t dstArrayElement)
{
	VkDescriptorBufferInfo info = {};
	info.buffer = buffer;
	info.offset = offset;
	info.range = size;

	writeBuffer(bindingIndex, type, info, dstArrayElement);

	return *this;
}

DescriptorWriter &DescriptorWriter::writeCombinedImage(uint32_t bindingIndex, VkDescriptorType type, const VkDescriptorImageInfo &info, uint32_t dstArrayElement)
{
	m_imageInfos[m_nImageInfos] = info;

	VkWriteDescriptorSet write = {};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.dstBinding = bindingIndex;
	write.dstSet = VK_NULL_HANDLE; // set on updateSet()
	write.dstArrayElement = dstArrayElement;
	write.descriptorCount = 1;
	write.descriptorType = type;
	write.pImageInfo = &m_imageInfos[m_nImageInfos];

	m_writes.pushBack(write);

	m_nImageInfos++;

	return *this;
}

DescriptorWriter &DescriptorWriter::writeCombinedImage(uint32_t bindingIndex, VkDescriptorType type, VkImageView image, VkImageLayout layout, VkSampler sampler, uint32_t dstArrayElement)
{
	VkDescriptorImageInfo info = {};
	info.imageView = image;
	info.imageLayout = layout;
	info.sampler = sampler;

	writeCombinedImage(bindingIndex, type, info, dstArrayElement);

	return *this;
}

DescriptorWriter &DescriptorWriter::writeSampledImage(uint32_t bindingIndex, VkImageView image, VkImageLayout layout, uint32_t dstArrayElement)
{
	return writeCombinedImage(bindingIndex, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, image, layout, VK_NULL_HANDLE, dstArrayElement);
}

DescriptorWriter &DescriptorWriter::writeSampler(uint32_t bindingIndex, VkSampler sampler, uint32_t dstArrayElement)
{
	return writeCombinedImage(bindingIndex, VK_DESCRIPTOR_TYPE_SAMPLER, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_UNDEFINED, sampler, dstArrayElement);
}
