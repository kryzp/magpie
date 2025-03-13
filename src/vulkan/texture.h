#ifndef VK_TEXTURE_H_
#define VK_TEXTURE_H_

#include "third_party/volk.h"
#include "third_party/vk_mem_alloc.h"

#include "core/common.h"

#include "container/vector.h"
#include "container/hash_map.h"

#include "rendering/image.h"

#include "gpu_buffer.h"
#include "texture_sampler.h"
#include "command_buffer.h"

namespace llt
{
	class RenderTarget;

	enum TextureProperty
	{
		TEXTURE_PROPERTY_MIPMAPPED,
		TEXTURE_PROPERTY_MAX_ENUM
	};

	class Texture
	{
	public:
		Texture();
		~Texture();

		void cleanUp();

		void fromImage(const Image &image, VkImageViewType type, uint32_t mipLevels, VkSampleCountFlagBits numSamples);

		void setSize(uint32_t width, uint32_t height);
		void setProperties(VkFormat format, VkImageTiling tiling, VkImageViewType type);
		void setMipLevels(uint32_t mipLevels);
		void setSampleCount(VkSampleCountFlagBits numSamples);
		void setTransient(bool isTransient);

		void flagAsDepthTexture();
		bool isDepthTexture() const;

		void createInternalResources();

		VkImageMemoryBarrier getBarrier() const;
		VkImageMemoryBarrier getBarrier(VkImageLayout newLayout) const;
		
		void transitionLayout(CommandBuffer &cmd, VkImageLayout newLayout);
		void transitionLayoutSingle(VkImageLayout newLayout);

		void generateMipmaps();

		void setParent(RenderTarget *getParent);
		const RenderTarget *getParent() const;
		bool hasParent() const;

		bool isUnorderedAccessView() const;
		void setUnorderedAccessView(bool uav);

		uint32_t getLayerCount() const;
		uint32_t getFaceCount() const;

		VkImage getImage() const;

		uint32_t getWidth() const;
		uint32_t getHeight() const;

		uint32_t getMipLevels() const;
		VkSampleCountFlagBits getNumSamples() const;
		bool isTransient() const;

		VkFormat getFormat() const;
		VkImageTiling getTiling() const;
		VkImageViewType getType() const;

		VkPipelineStageFlags getStage() const;

		VkImageView getStandardView() const;
		VkImageView createView(int layerCount, int layer, int baseMipLevel) const;

		VkImageLayout getImageLayout() const;

	private:
		RenderTarget *m_parent;

		VkImage m_image;
		VkImageView m_standardView;
		VkImageLayout m_imageLayout;
		uint32_t m_mipmapCount;
		VkSampleCountFlagBits m_numSamples;
		bool m_transient;

		VkPipelineStageFlags m_stage;

		VmaAllocation m_allocation;
		VmaAllocationInfo m_allocationInfo;

		VkFormat m_format;
		VkImageTiling m_tiling;
		VkImageViewType m_type;

		uint32_t m_width;
		uint32_t m_height;

		uint32_t m_depth;
		bool m_isDepthTexture;

		bool m_uav;
	};

	class BoundTexture
	{
	public:
		BoundTexture()
			: m_texture(nullptr)
			, m_sampler(nullptr)
		{
		}

		BoundTexture(Texture *texture, TextureSampler *sampler)
			: m_texture(texture)
			, m_sampler(sampler)
		{
		}

		VkDescriptorImageInfo getStandardImageInfo() const
		{
			VkDescriptorImageInfo info = {};

			info.imageView = m_texture->getStandardView();

			info.imageLayout = m_texture->isDepthTexture()
				? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
				: VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			info.sampler = m_sampler->bind(m_texture->getMipLevels());

			return info;
		}

		VkDescriptorImageInfo getImageInfo(VkImageView view) const
		{
			VkDescriptorImageInfo info = {};

			info.imageView = view;

			info.imageLayout = m_texture->isDepthTexture()
				? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
				: VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			info.sampler = m_sampler->bind(m_texture->getMipLevels());

			return info;
		}

	private:
		Texture *m_texture;
		TextureSampler *m_sampler;
	};
}

#endif // VK_TEXTURE_H_
