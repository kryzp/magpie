#pragma once

#include <inttypes.h>

#include <vector>
#include <array>
#include <unordered_map>

#include <Volk/volk.h>

namespace mgp
{
	class GraphicsCore;
	class GPUBuffer;
	class ImageView;
	class Sampler;

	struct DescriptorPoolSize
	{
		VkDescriptorType type;
		float max;
	};

	struct DescriptorLayoutBinding
	{
		uint32_t index;
		VkDescriptorType type;
		uint32_t count;
		VkDescriptorBindingFlags bindingFlags;

		DescriptorLayoutBinding(uint32_t index, VkDescriptorType type, uint32_t count = 1, VkDescriptorBindingFlags bindingFlags = 0)
			: index(index)
			, type(type)
			, count(count)
			, bindingFlags(bindingFlags)
		{
		}
	};

	class DescriptorLayout
	{
	public:
		DescriptorLayout(GraphicsCore *gfx, VkDescriptorSetLayout layout);
		~DescriptorLayout();

		VkDescriptorSetLayout getLayout() const { return m_layout; }

	private:
		GraphicsCore *m_gfx;
		VkDescriptorSetLayout m_layout;
	};

	class DescriptorLayoutCache
	{
	public:
		DescriptorLayoutCache() = default;
		~DescriptorLayoutCache() = default;

		void init(GraphicsCore *gfx);
		void destroy();

		DescriptorLayout *fetchLayout(
			VkShaderStageFlags shaderStages,
			const std::vector<DescriptorLayoutBinding> &bindings,
			VkDescriptorSetLayoutCreateFlags flags
		);

	private:
		GraphicsCore *m_gfx;
		std::unordered_map<uint64_t, DescriptorLayout *> m_layoutCache;
	};

	class Descriptor
	{
		constexpr static int MAX_BUFFER_INFOS = 32;
		constexpr static int MAX_IMAGE_INFOS = 32;

	public:
		Descriptor(GraphicsCore *gfx, const VkDescriptorSet &set);
		~Descriptor() = default;

		void clear();

		Descriptor *writeBuffer(uint32_t bindingIndex, const GPUBuffer *buffer, bool dynamic, uint32_t arrayIndex = 0);
		Descriptor *writeBufferRange(uint32_t bindingIndex, const GPUBuffer *buffer, bool dynamic, uint32_t range, uint32_t arrayIndex = 0);

		Descriptor *writeCombinedImage(uint32_t bindingIndex, const ImageView *view, const Sampler *sampler, uint32_t arrayIndex = 0);
		Descriptor *writeSampledImage(uint32_t bindingIndex, const ImageView *view, uint32_t arrayIndex = 0);
		Descriptor *writeStorageImage(uint32_t bindingIndex, const ImageView *view, uint32_t arrayIndex = 0);
		Descriptor *writeSampler(uint32_t bindingIndex, const Sampler *sampler, uint32_t arrayIndex = 0);

		Descriptor *writeBufferVulkan(uint32_t bindingIndex, VkDescriptorType type, const VkDescriptorBufferInfo &info, uint32_t arrayIndex);
		Descriptor *writeImageVulkan(uint32_t bindingIndex, VkDescriptorType type, const VkDescriptorImageInfo &info, uint32_t arrayIndex);

		const VkDescriptorSet &getHandle();
	
	private:
		GraphicsCore *m_gfx;

		void update();

		VkDescriptorSet m_set;

		std::vector<VkWriteDescriptorSet> m_writes;

		bool m_dirty;

		std::array<VkDescriptorBufferInfo, MAX_BUFFER_INFOS> m_bufferInfos;
		std::array<VkDescriptorImageInfo, MAX_IMAGE_INFOS> m_imageInfos;

		int m_nBufferInfos;
		int m_nImageInfos;
	};

	class DescriptorPoolStatic
	{
	public:
		DescriptorPoolStatic(GraphicsCore *gfx, uint32_t maxSets, VkDescriptorPoolCreateFlags flags, const std::vector<DescriptorPoolSize> &sizes);
		~DescriptorPoolStatic();

		void clear();
		Descriptor *allocate(const std::vector<DescriptorLayout *> &layouts);

		VkDescriptorPool getPool() const;

	private:
		GraphicsCore *m_gfx;
		VkDescriptorPool m_pool;

		std::vector<Descriptor *> m_allocated;
	};

	class DescriptorPool
	{
		constexpr static float GROWTH_FACTOR = 2.0f; // grows by 2x every time
		constexpr static uint32_t MAX_SETS = 4092; // hard limit to prevent infinite growth

	public:
		DescriptorPool(GraphicsCore *gfx, uint32_t initialSets, VkDescriptorPoolCreateFlags flags, const std::vector<DescriptorPoolSize> &sizes);
		~DescriptorPool();

		void clear();
		Descriptor *allocate(const std::vector<DescriptorLayout *> &layouts);

	private:
		GraphicsCore *m_gfx;

		VkDescriptorPool fetchPool();
		VkDescriptorPool createNewPool(uint32_t maxSets);

		std::vector<VkDescriptorPool> m_usedPools;
		std::vector<VkDescriptorPool> m_freePools;

		uint32_t m_setsPerPool;

		VkDescriptorPoolCreateFlags m_flags;
		std::vector<DescriptorPoolSize> m_sizes;

		std::vector<Descriptor *> m_allocated;
	};
}
