#ifndef DESCRIPTOR_BUILDER_H_
#define DESCRIPTOR_BUILDER_H_

#include "third_party/volk.h"

#include "container/deque.h"
#include "container/vector.h"

namespace llt
{
	class DescriptorLayoutCache;

	class DescriptorLayoutBuilder
	{
	public:
		DescriptorLayoutBuilder() = default;
		~DescriptorLayoutBuilder() = default;

		VkDescriptorSetLayout build(VkShaderStageFlags shaderStages, void *pNext = nullptr, VkDescriptorSetLayoutCreateFlags flags = 0);

		DescriptorLayoutBuilder &bind(uint32_t idx, VkDescriptorType type);

	private:
		Vector<VkDescriptorSetLayoutBinding> m_bindings;
	};

	class DescriptorWriter
	{
	public:
		DescriptorWriter() = default;
		~DescriptorWriter() = default;

		DescriptorWriter &updateSet(const VkDescriptorSet &set);

		DescriptorWriter &writeBuffer(uint32_t idx, VkDescriptorType type, const VkDescriptorBufferInfo &info);
		DescriptorWriter &writeBuffer(uint32_t idx, VkDescriptorType type, VkBuffer buffer, uint64_t size, uint64_t offset);

		DescriptorWriter &writeImage(uint32_t idx, VkDescriptorType type, const VkDescriptorImageInfo &info);
		DescriptorWriter &writeImage(uint32_t idx, VkDescriptorType type, VkImageView image, VkSampler sampler, VkImageLayout layout);

	private:
		Vector<VkWriteDescriptorSet> m_writes;

		Vector<VkDescriptorBufferInfo> m_bufferInfos;
		Vector<VkDescriptorImageInfo> m_imageInfos;
	};
}

#endif // DESCRIPTOR_BUILDER_H_
