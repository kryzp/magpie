#ifndef VK_TEXTURE_H_
#define VK_TEXTURE_H_

#include "third_party/volk.h"
#include "third_party/vk_mem_alloc.h"

#include "core/common.h"

#include "container/vector.h"
#include "container/hash_map.h"

#include "texture_sampler.h"
#include "texture_view.h"

namespace llt
{
	class CommandBuffer;
	class RenderTarget;
	class Image;

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

		VkImageMemoryBarrier2 getBarrier() const;
		VkImageMemoryBarrier2 getBarrier(VkImageLayout newLayout) const;
		
		void transitionLayout(CommandBuffer &cmd, VkImageLayout newLayout);
		void transitionLayoutSingle(VkImageLayout newLayout);

		void generateMipmaps(CommandBuffer &cmd);

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
		VkImageLayout getImageLayout() const;

		TextureView getStandardView();
		TextureView getView(int layerCount, int layer, int baseMipLevel);

		VkPipelineStageFlags getStage() const;

	private:
		RenderTarget *m_parent;

		VkImage m_image;
		VkImageLayout m_imageLayout;
		uint32_t m_mipmapCount;
		VkSampleCountFlagBits m_numSamples;
		bool m_transient;

		HashMap<uint64_t, TextureView> m_viewCache;

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

		bool m_isUAV;
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

		VkDescriptorImageInfo getImageInfo(const TextureView &view) const
		{
			VkDescriptorImageInfo info = {};

			info.imageView = view.getHandle();

			info.imageLayout = m_texture->isDepthTexture()
				? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
				: VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			info.sampler = m_sampler->getHandle();

			return info;
		}

		VkDescriptorImageInfo getStandardImageInfo() const
		{
			return getImageInfo(m_texture->getStandardView());
		}

	private:
		Texture *m_texture;
		TextureSampler *m_sampler;
	};
}

#endif // VK_TEXTURE_H_
