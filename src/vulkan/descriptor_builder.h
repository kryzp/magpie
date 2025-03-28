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

		DescriptorLayoutBuilder &bind(uint32_t bindingIndex, VkDescriptorType type, uint32_t descriptorCount = 1);

	private:
		Vector<VkDescriptorSetLayoutBinding> m_bindings;
	};

	class DescriptorWriter
	{
	public:
		DescriptorWriter() = default;
		~DescriptorWriter() = default;

		void clear();
		void updateSet(const VkDescriptorSet &set);

		DescriptorWriter &writeBuffer(uint32_t bindingIndex, VkDescriptorType type, const VkDescriptorBufferInfo &info, uint32_t dstArrayElement = 0);
		DescriptorWriter &writeBuffer(uint32_t bindingIndex, VkDescriptorType type, VkBuffer buffer, uint64_t size, uint64_t offset, uint32_t dstArrayElement = 0);

		DescriptorWriter &writeCombinedImage(uint32_t bindingIndex, VkDescriptorType type, const VkDescriptorImageInfo &info, uint32_t dstArrayElement = 0);
		DescriptorWriter &writeCombinedImage(uint32_t bindingIndex, VkDescriptorType type, VkImageView image, VkImageLayout layout, VkSampler sampler, uint32_t dstArrayElement = 0);

		DescriptorWriter &writeSampledImage(uint32_t bindingIndex, VkImageView image, VkImageLayout layout, uint32_t dstArrayElement = 0);
		DescriptorWriter &writeSampler(uint32_t bindingIndex, VkSampler sampler, uint32_t dstArrayElement = 0);

	private:
		Vector<VkWriteDescriptorSet> m_writes;

		VkDescriptorBufferInfo m_bufferInfos[128];
		VkDescriptorImageInfo m_imageInfos[128];

		int m_nBufferInfos;
		int m_nImageInfos;
	};
}

#endif // DESCRIPTOR_BUILDER_H_
