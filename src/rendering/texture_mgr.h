#ifndef VK_TEXTURE_MGR_H_
#define VK_TEXTURE_MGR_H_

#include "container/vector.h"
#include "container/hash_map.h"

#include "vulkan/texture.h"

namespace llt
{
	class Texture;
	class TextureSampler;
	class Image;

	class TextureMgr
	{
	public:
		TextureMgr();
		~TextureMgr();

		void loadDefaultTexturesAndSamplers();

		Texture *getTexture(const String &name);
		TextureSampler *getSampler(const String &name);

		Texture *load(const String &name, const String &path);
		
		Texture *createFromImage(const String &name, const Image &image);
		Texture *createFromData(const String &name, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, const byte *data, uint64_t size);
		Texture *createAttachment(const String &name, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling);
		
		Texture *createCubemap(const String &name, uint32_t size, VkFormat format, int mipLevels);
		Texture *createCubemap(const String &name, const Image &right, const Image &left, const Image &top, const Image &bottom, const Image &front, const Image &back, int mipLevels);

		TextureSampler *createSampler(const String &name, const TextureSampler::Style &style);

	private:
		HashMap<String, Texture*> m_textureCache;
		HashMap<String, TextureSampler*> m_samplerCache;
	};

	extern TextureMgr *g_textureManager;
}

#endif // VK_TEXTURE_MGR_H_
