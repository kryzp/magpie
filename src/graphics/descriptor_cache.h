#ifndef DESCRIPTOR_CACHER_H_
#define DESCRIPTOR_CACHER_H_

#include <vulkan/vulkan.h>

#include "../container/hash_map.h"
#include "descriptor_pool_mgr.h"

namespace llt
{
	class VulkanBackend;
	class DescriptorPoolMgr;

	class DescriptorCache
	{
	public:
		DescriptorCache();
		~DescriptorCache();

		void setPoolManager(DescriptorPoolMgr& poolMgr);

		void cleanUp();
		void clearSetCache();

		VkDescriptorSet createSet(const VkDescriptorSetLayout& layout, uint32_t count, uint64_t hash, bool* wasAlreadyCached);
		VkDescriptorSetLayout createLayout(const VkDescriptorSetLayoutCreateInfo& layoutCreateInfo);

	private:
		HashMap<uint64_t, VkDescriptorSet> m_descriptorCache;
		HashMap<uint64_t, VkDescriptorSetLayout> m_layoutCache;

		DescriptorPoolMgr* m_poolMgr;
	};
}

#endif // DESCRIPTOR_CACHER_H_
