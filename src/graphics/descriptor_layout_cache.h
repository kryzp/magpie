#ifndef DESCRIPTOR_LAYOUT_CACHE_H_
#define DESCRIPTOR_LAYOUT_CACHE_H_

#include <vulkan/vulkan.h>

#include "../container/hash_map.h"

namespace llt
{
	class DescriptorLayoutCache
	{
	public:
		DescriptorLayoutCache() = default;
		~DescriptorLayoutCache() = default;

		void cleanUp();

		VkDescriptorSetLayout createLayout(const VkDescriptorSetLayoutCreateInfo& layoutCreateInfo);

	private:
		HashMap<uint64_t, VkDescriptorSetLayout> m_layoutCache;
	};
}

#endif // DESCRIPTOR_LAYOUT_CACHE_H_
