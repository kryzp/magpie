#ifndef DESCRIPTOR_CACHER_H_
#define DESCRIPTOR_CACHER_H_

#include <vulkan/vulkan.h>

#include "../container/hash_map.h"

namespace llt
{
	class VulkanBackend;
	class DescriptorPoolMgr;

	class DescriptorCache
	{
	public:
		DescriptorCache();
		~DescriptorCache();

		void cleanUp();
		void clearSetCache();

		VkDescriptorSet createSet(const VkDescriptorSetLayout& layout, uint64_t hash, bool* wasAlreadyCached);
		VkDescriptorSetLayout createLayout(const VkDescriptorSetLayoutCreateInfo& layoutCreateInfo);

	private:
		HashMap<uint64_t, VkDescriptorSet> m_descriptorCache;
		HashMap<uint64_t, VkDescriptorSetLayout> m_layoutCache;
	};
}

#endif // DESCRIPTOR_CACHER_H_
