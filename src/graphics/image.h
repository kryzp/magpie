#pragma once

#include <Volk/volk.h>
#include <vma/vk_mem_alloc.h>

namespace mgp
{
	class ImageView;
	class GraphicsCore;

	class Image
	{
		friend class CommandBuffer;

	public:
		Image() = default;
		~Image();

		void allocate(
			GraphicsCore *gfx,
			unsigned width, unsigned height, unsigned depth,
			VkFormat format,
			VkImageViewType type,
			VkImageTiling tiling,
			uint32_t mipmaps,
			VkSampleCountFlagBits samples,
			bool transient,
			bool storage
		);

		void wrapAround(
			GraphicsCore *gfx,
			VkImage image,
			VkImageLayout layout,
			unsigned width, unsigned height, unsigned depth,
			VkFormat format,
			VkImageViewType type,
			VkImageTiling tiling,
			uint32_t mipmaps,
			VkSampleCountFlagBits samples,
			VkImageUsageFlags usage
		);

		uint32_t getWidth() const { return m_width; }
		uint32_t getHeight() const { return m_height; }
		uint32_t getDepth() const { return m_depth; }

		VkFormat getFormat() const { return m_format; }
		VkImageViewType getType() const { return m_type; }
		VkImageTiling getTiling() const { return m_tiling; }
		
		uint32_t getMipmapCount() const;
		VkSampleCountFlagBits getSamples() const;

		bool isTransient() const;
		bool isStorage() const;

		bool isCubemap() const;
		bool isDepth() const;

		unsigned getLayerCount() const;
		unsigned getFaceCount() const;

		VkImageMemoryBarrier2 getBarrier(VkImageLayout newLayout) const;

		const VkImage &getHandle() const;
		const VkImageLayout &getLayout() const;

		VkImageUsageFlags getUsage() const;

	private:
		GraphicsCore *m_gfx;

		VkImage m_image;
		VkImageLayout m_layout;
		
		VmaAllocation m_allocation;
		VmaAllocationInfo m_allocationInfo;
		bool m_isAllocated;

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
