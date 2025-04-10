#pragma once

#include <unordered_map>

#include "third_party/volk.h"
#include "third_party/vk_mem_alloc.h"

namespace mgp
{
	class VulkanCore;
	class ImageView;
	class Image;

	class ImageAlloc
	{
	public:
		ImageAlloc(Image *image);
		~ImageAlloc();

	private:
		Image *m_parent;

		VmaAllocation m_allocation;
		VmaAllocationInfo m_allocationInfo;
	};

	class Image
	{
		friend class ImageAlloc;
		friend class Swapchain;
		friend class CommandBuffer;

	public:
		Image() = default;

		Image(
			VulkanCore *core,
			unsigned width, unsigned height, unsigned depth,
			VkFormat format,
			VkImageViewType type,
			VkImageTiling tiling,
			uint32_t mipmaps,
			VkSampleCountFlagBits samples,
			bool transient,
			bool storage
		);

		~Image();

		void init(
			VulkanCore *core,
			unsigned width, unsigned height, unsigned depth,
			VkFormat format,
			VkImageViewType type,
			VkImageTiling tiling,
			uint32_t mipmaps,
			VkSampleCountFlagBits samples,
			bool transient,
			bool storage
		);

		void allocate();

		ImageView *createView(
			int layerCount,
			int layer,
			int baseMipLevel
		);

		ImageView *getStandardView();

		VkImageMemoryBarrier2 getBarrier(VkImageLayout newLayout) const;

		const VkImage &getHandle() const;

		const VkImageLayout &getLayout() const;

		unsigned getWidth() const;
		unsigned getHeight() const;
		unsigned getDepth() const;

		VkFormat getFormat() const;
		VkImageViewType getType() const;
		VkImageTiling getTiling() const;

		VkImageUsageFlags getUsage() const;

		uint32_t getMipmapCount() const;
		VkSampleCountFlagBits getSamples() const;

		bool isTransient() const;
		bool isStorage() const;

		bool isCubemap() const;
		bool isDepth() const;

		unsigned getLayerCount() const;
		unsigned getFaceCount() const;

	private:
		VkImage m_image;
		VkImageLayout m_layout;

		VulkanCore *m_core;

		std::unordered_map<uint64_t, ImageView *> m_viewCache;

		ImageAlloc *m_allocation;

		unsigned m_width;
		unsigned m_height;
		unsigned m_depth;

		VkFormat m_format;
		VkImageViewType m_type;
		VkImageTiling m_tiling;

		VkImageUsageFlags m_usage;

		uint32_t m_mipmapCount;
		VkSampleCountFlagBits m_samples;
	};
}
