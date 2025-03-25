#ifndef VK_DESCRIPTOR_ALLOCATOR
#define VK_DESCRIPTOR_ALLOCATOR

#include "third_party/volk.h"

#include "container/vector.h"
#include "container/pair.h"

namespace llt
{
	using DescriptorPoolSizeRatio = Pair<VkDescriptorType, float>;

	// just a wrapper around a static descriptor pool
	class DescriptorPoolStatic
	{
	public:
		DescriptorPoolStatic() = default;
		~DescriptorPoolStatic() = default;

		void init(uint32_t maxSets, VkDescriptorPoolCreateFlags flags, const Vector<DescriptorPoolSizeRatio> &sizes);
		void cleanUp();

		void clear();

		VkDescriptorSet allocate(VkDescriptorSetLayout layout);

		VkDescriptorPool getPool() const;

	private:
		VkDescriptorPool m_pool;
	};

	class DescriptorPoolDynamic
	{
		constexpr static float GROWTH_FACTOR = 2.0f; // grows by 2x every time
		constexpr static uint32_t MAX_SETS = 4092; // hard limit to prevent infinate growth

	public:
		DescriptorPoolDynamic() = default;
		~DescriptorPoolDynamic() = default;

		void init(uint32_t initialSets, const Vector<DescriptorPoolSizeRatio> &sizes);

		void cleanUp();

		void clear();

		VkDescriptorSet allocate(const VkDescriptorSetLayout &layout, void *pNext = nullptr);

	private:
		VkDescriptorPool fetchPool();
		VkDescriptorPool createNewPool(uint32_t setCount, const Vector<DescriptorPoolSizeRatio> &sizes);

		Vector<VkDescriptorPool> m_usedPools;
		Vector<VkDescriptorPool> m_freePools;

		uint32_t m_setsPerPool;

		Vector<DescriptorPoolSizeRatio> m_sizes;
	};
}

#endif // VK_DESCRIPTOR_ALLOCATOR
