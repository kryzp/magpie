#pragma once

#include <vector>
#include <unordered_map>
#include <utility>

#include "third_party/volk.h"

namespace mgp
{
	class VulkanCore;

	using DescriptorPoolSizeRatio = std::pair<VkDescriptorType, float>;

	class DescriptorPoolStatic
	{
	public:
		DescriptorPoolStatic() = default;
		~DescriptorPoolStatic() = default;

		void init(const VulkanCore *core, uint32_t maxSets, VkDescriptorPoolCreateFlags flags, const std::vector<DescriptorPoolSizeRatio> &sizes);

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

		void init(const VulkanCore *core, uint32_t initialSets, const std::vector<DescriptorPoolSizeRatio> &sizes);

		void cleanUp();

		void clear();

		VkDescriptorSet allocate(const std::vector<VkDescriptorSetLayout> &layouts, void *pNext = nullptr);

	private:
		VkDescriptorPool fetchPool();
		VkDescriptorPool createNewPool(uint32_t setCount, const std::vector<DescriptorPoolSizeRatio> &sizes);

		std::vector<VkDescriptorPool> m_usedPools;
		std::vector<VkDescriptorPool> m_freePools;

		uint32_t m_setsPerPool;

		std::vector<DescriptorPoolSizeRatio> m_sizes;

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

		DescriptorWriter &writeBuffer(uint32_t bindingIndex, VkDescriptorType type, const VkDescriptorBufferInfo &info, uint32_t dstArrayElement = 0);
		DescriptorWriter &writeBuffer(uint32_t bindingIndex, VkDescriptorType type, VkBuffer buffer, uint64_t size, uint64_t offset, uint32_t dstArrayElement = 0);

		DescriptorWriter &writeCombinedImage(uint32_t bindingIndex, VkDescriptorType type, const VkDescriptorImageInfo &info, uint32_t dstArrayElement = 0);
		DescriptorWriter &writeCombinedImage(uint32_t bindingIndex, VkDescriptorType type, VkImageView image, VkImageLayout layout, VkSampler sampler, uint32_t dstArrayElement = 0);

		DescriptorWriter &writeSampledImage(uint32_t bindingIndex, VkImageView image, VkImageLayout layout, uint32_t dstArrayElement = 0);
		DescriptorWriter &writeStorageImage(uint32_t bindingIndex, VkImageView image, VkImageLayout layout, uint32_t dstArrayElement = 0);
		DescriptorWriter &writeSampler(uint32_t bindingIndex, VkSampler sampler, uint32_t dstArrayElement = 0);

	private:
		std::vector<VkWriteDescriptorSet> m_writes;

		VkDescriptorBufferInfo m_bufferInfos[128];
		VkDescriptorImageInfo m_imageInfos[128];

		int m_nBufferInfos;
		int m_nImageInfos;
	};
}
