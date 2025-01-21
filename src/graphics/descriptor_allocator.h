#ifndef VK_DESCRIPTOR_ALLOCATOR
#define VK_DESCRIPTOR_ALLOCATOR

#include "../third_party/volk.h"

#include "../container/vector.h"
#include "../container/pair.h"

namespace llt
{
	using DescriptorPoolSizeRatio = Pair<VkDescriptorType, float>;

	// just a wrapper around a static descriptor pool
	class DescriptorPoolStatic
	{
	public:
		DescriptorPoolStatic() = default;
		~DescriptorPoolStatic() = default;

		void init(uint32_t maxSets, const Vector<DescriptorPoolSizeRatio>& sizes);
		void clear();

		void cleanUp();

		VkDescriptorSet allocate(VkDescriptorSetLayout layout);

	private:
		VkDescriptorPool m_pool;
	};

	// acts like a regular pool but dynamically grows to fill up more space if it runs out of room, like a vector
	class DescriptorPoolDynamic
	{
		constexpr static float GROWTH_FACTOR = 2.0f; // grows by 2x every time
		constexpr static uint32_t MAX_SETS = 4092; // hard limit to prevent infinate growth

	public:
		DescriptorPoolDynamic() = default;
		~DescriptorPoolDynamic() = default;

		void setSizes(uint32_t initialSets, const Vector<DescriptorPoolSizeRatio>& sizes);
		void clear();

		void cleanUp();

		VkDescriptorSet allocate(const VkDescriptorSetLayout& layout, void* pNext = nullptr);

	private:
		VkDescriptorPool fetchPool();
		VkDescriptorPool createNewPool(uint32_t setCount, const Vector<DescriptorPoolSizeRatio>& sizes);

		Vector<VkDescriptorPool> m_usedPools;
		Vector<VkDescriptorPool> m_freePools;

		uint32_t m_setsPerPool;

		Vector<DescriptorPoolSizeRatio> m_sizes;
	};
}

#endif // VK_DESCRIPTOR_ALLOCATOR
