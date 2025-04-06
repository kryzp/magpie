#pragma once

#include <unordered_map>

#include "third_party/volk.h"
#include "third_party/vk_mem_alloc.h"

namespace mgp
{
	class VulkanCore;
	class ImageView;

	/*
		vulkan image wrapper
	*/
	class ImageInfo
	{
		friend class Image;
		friend class Swapchain;

		friend class CommandBuffer;

	public:
		ImageInfo();
		~ImageInfo();

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

		unsigned m_width;
		unsigned m_height;
		unsigned m_depth;

		VkFormat m_format;
		VkImageViewType m_type;
		VkImageTiling m_tiling;

		VkImageUsageFlags m_usage;

		uint32_t m_mipmapCount;
		VkSampleCountFlagBits m_samples;

		bool m_transient;
		bool m_storage;
	};

	/*
		memory allocated image
	*/
	class Image
	{
		friend class CommandBuffer;

	public:
		Image();

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

		void create(
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

		ImageView *createView(
			int layerCount,
			int layer,
			int baseMipLevel
		);

		ImageView *getStandardView();

		const VkImage &getHandle() const;

		ImageInfo &getInfo();
		const ImageInfo &getInfo() const;

	private:
		ImageInfo m_info;
		std::unordered_map<uint64_t, ImageView *> m_viewCache;

		VmaAllocation m_allocation;
		VmaAllocationInfo m_allocationInfo;

		VulkanCore *m_core;
	};
}
