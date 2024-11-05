#ifndef VK_TEXTURE_H_
#define VK_TEXTURE_H_

#include <vulkan/vulkan.h>

#include "buffer.h"
#include "texture_sampler.h"
#include "image.h"

#include "../common.h"

namespace llt
{
	class RenderTarget;

	enum TextureProperty
	{
		TEXTURE_PROPERTY_NONE = 0,
		TEXTURE_PROPERTY_MIPMAPPED,
		TEXTURE_PROPERTY_MAX_ENUM
	};

	struct TextureInfo
	{
		VkFormat format;
		VkImageTiling tiling;
		VkImageViewType type;
	};

	class Texture
	{
	public:
		Texture();
		~Texture();

		void cleanUp();

		void fromImage(const Image& image, VkImageViewType type, uint32_t mipLevels, VkSampleCountFlagBits numSamples);

		void initSize(uint32_t width, uint32_t height);
		void initMetadata(VkFormat format, VkImageTiling tiling, VkImageViewType type);
		void initMipLevels(uint32_t mipLevels);
		void initSampleCount(VkSampleCountFlagBits numSamples);
		void initTransient(bool isTransient);

		void createInternalResources();

		void transitionLayout(VkImageLayout newLayout);
		void generateMipmaps() const;

		void setParent(RenderTarget* getParent);
		const RenderTarget* getParent() const;
		bool hasParent() const;

		bool isMipmapped() const;
		void setMipmapped(bool mipmapped);

		bool isUnorderedAccessView() const;
		void setUnorderedAccessView(bool uav);

		uint32_t getLayerCount() const;
		uint32_t getFaceCount() const;

		VkImage getImage() const;
		VkImageView getImageView() const;

		uint32_t width() const;
		uint32_t height() const;

		uint32_t getMipLevels() const;
		VkSampleCountFlagBits getNumSamples() const;
		bool isTransient() const;

		TextureInfo getInfo() const;

	private:
		VkImageView generate_view() const;

		RenderTarget* m_parent;

		VkImage m_image;
		VkDeviceMemory m_imageMemory;
		VkImageLayout m_imageLayout;
		VkImageView m_view;
		uint32_t m_mipmapCount;
		VkSampleCountFlagBits m_numSamples;
		bool m_transient;

		VkFormat m_format;
		VkImageTiling m_tiling;
		VkImageViewType m_type;

		uint32_t m_width;
		uint32_t m_height;

		uint32_t m_depth;
		bool m_mipmapped;

		bool m_uav;
	};

	/**
	* Wrapper around a texture and sampler pair
	*/
	class SampledTexture
	{
	public:
		SampledTexture()
			: texture(nullptr)
			, sampler(nullptr)
		{
		}

		SampledTexture(const Texture* texture, TextureSampler* sampler)
			: texture(texture)
			, sampler(sampler)
		{
		}

		~SampledTexture() = default;

		const Texture* texture;
		TextureSampler* sampler;
	};
}

#endif // VK_TEXTURE_H_
