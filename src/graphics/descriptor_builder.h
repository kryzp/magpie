#ifndef DESCRIPTOR_BUILDER_H_
#define DESCRIPTOR_BUILDER_H_

#include "descriptor_pool_mgr.h"
#include "descriptor_cache.h"

#include "../container/vector.h"

namespace llt
{
	class DescriptorBuilder
	{
	public:
		DescriptorBuilder() = default;
		~DescriptorBuilder() = default;

		void clear();

		uint64_t getHash() const;

		void build(VkDescriptorSet& set, VkDescriptorSetLayout& layout, uint64_t hash);
		void buildLayout(VkDescriptorSetLayout& layout);

		void bindBuffer(uint32_t idx, const VkDescriptorBufferInfo* info, VkDescriptorType type, VkShaderStageFlags stageFlags);
		void bindImage(uint32_t idx, const VkDescriptorImageInfo* info, VkDescriptorType type, VkShaderStageFlags stageFlags);

	private:
		Vector<VkWriteDescriptorSet> m_writes;
		Vector<VkDescriptorSetLayoutBinding> m_bindings;
	};
}

#endif // DESCRIPTOR_BUILDER_H_
