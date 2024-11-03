#ifndef VK_DESCRIPTOR_POOL_MGR
#define VK_DESCRIPTOR_POOL_MGR

#include <vulkan/vulkan.h>

#include "../container/vector.h"
#include "../container/pair.h"

namespace llt
{
	class VulkanBackend;

	class DescriptorPoolMgr
	{
	public:
		DescriptorPoolMgr();
		~DescriptorPoolMgr();

		void init();

		void cleanUp();
		void resetPools();

		VkDescriptorSet allocateDescriptorSet(const VkDescriptorSetLayout& layout);

		VkDescriptorPool fetchPool();

	private:
		VkDescriptorPool createNewPool(uint32_t count);
		void initSizes();

		Vector<VkDescriptorPool> m_usedPools;
		Vector<VkDescriptorPool> m_freePools;

		VkDescriptorPool m_currentPool;

		Vector<Pair<VkDescriptorType, float>> m_sizes;
	};
}

#endif // VK_DESCRIPTOR_POOL_MGR
