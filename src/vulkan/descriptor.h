#pragma once

#include <vector>
#include <unordered_map>
#include <utility>

#include "third_party/volk.h"

namespace mgp
{
	class VulkanCore;
	class ImageView;
	class Sampler;
	class GPUBuffer;

	struct DescriptorPoolSize
	{
		VkDescriptorType type;
		uint32_t max;
	};

	class DescriptorPoolStatic
	{
	public:
		DescriptorPoolStatic() = default;
		~DescriptorPoolStatic() = default;

		void init(const VulkanCore *core, uint32_t maxSets, VkDescriptorPoolCreateFlags flags, const std::vector<DescriptorPoolSize> &sizes);

		void cleanUp();

		void clear();

		VkDescriptorSet allocate(VkDescriptorSetLayout layout);

		VkDescriptorPool getPool() const;

	private:
		VkDescriptorPool m_pool;

		const VulkanCore *m_core;
	};

	class DescriptorPoolDynamic
	{
		constexpr static float GROWTH_FACTOR = 2.0f; // grows by 2x every time
		constexpr static uint32_t MAX_SETS = 4092; // hard limit to prevent infinite growth

	public:
		DescriptorPoolDynamic() = default;
		~DescriptorPoolDynamic() = default;

		void init(const VulkanCore *core, uint32_t initialSets, const std::vector<DescriptorPoolSize> &sizes);

		void cleanUp();

		void clear();

		VkDescriptorSet allocate(const std::vector<VkDescriptorSetLayout> &layouts, void *pNext = nullptr);

	private:
		VkDescriptorPool fetchPool();
		VkDescriptorPool createNewPool(uint32_t maxSets, const std::vector<DescriptorPoolSize> &sizes);

		std::vector<VkDescriptorPool> m_usedPools;
		std::vector<VkDescriptorPool> m_freePools;

		uint32_t m_setsPerPool;

		std::vector<DescriptorPoolSize> m_sizes;

		const VulkanCore *m_core;
	};

	class DescriptorLayoutCache
	{
	public:
		DescriptorLayoutCache() = default;
		~DescriptorLayoutCache() = default;

		void init(const VulkanCore *core);
		void dispose();

		VkDescriptorSetLayout createLayout(const VkDescriptorSetLayoutCreateInfo &layoutCreateInfo);

	private:
		std::unordered_map<uint64_t, VkDescriptorSetLayout> m_layoutCache;

		const VulkanCore *m_core;
	};

	class DescriptorLayoutBuilder
	{
	public:
		DescriptorLayoutBuilder() = default;
		~DescriptorLayoutBuilder() = default;

		VkDescriptorSetLayout build(VulkanCore *core, VkShaderStageFlags shaderStages, void *pNext = nullptr, VkDescriptorSetLayoutCreateFlags flags = 0);

		DescriptorLayoutBuilder &bind(uint32_t bindingIndex, VkDescriptorType type, uint32_t descriptorCount = 1);

	private:
		std::vector<VkDescriptorSetLayoutBinding> m_bindings;
	};

	class DescriptorWriter
	{
	public:
		DescriptorWriter() = default;
		~DescriptorWriter() = default;

		void clear();
		void updateSet(const VulkanCore *core, const VkDescriptorSet &set);

		DescriptorWriter &writeBuffer(uint32_t bindingIndex, VkDescriptorType type, const VkDescriptorBufferInfo &info, uint32_t arrayIndex = 0);
//		DescriptorWriter &writeBuffer(uint32_t bindingIndex, const GPUBuffer &buffer, bool dynamic, uint32_t arrayIndex = 0);

		DescriptorWriter &writeImage(uint32_t bindingIndex, VkDescriptorType type, const VkDescriptorImageInfo &info, uint32_t arrayIndex = 0);
		DescriptorWriter &writeCombinedImage(uint32_t bindingIndex, const ImageView &view, const Sampler &sampler, uint32_t arrayIndex = 0);
		DescriptorWriter &writeSampledImage(uint32_t bindingIndex, const ImageView &view, uint32_t arrayIndex = 0);
		DescriptorWriter &writeStorageImage(uint32_t bindingIndex, const ImageView &view, uint32_t arrayIndex = 0);
		DescriptorWriter &writeSampler(uint32_t bindingIndex, const Sampler &sampler, uint32_t arrayIndex = 0);

	private:
		std::vector<VkWriteDescriptorSet> m_writes;

		VkDescriptorBufferInfo m_bufferInfos[128];
		VkDescriptorImageInfo m_imageInfos[128];

		int m_nBufferInfos;
		int m_nImageInfos;
	};
}
