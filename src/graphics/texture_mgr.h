#ifndef VK_TEXTURE_MGR_H_
#define VK_TEXTURE_MGR_H_

#include "../container/vector.h"
#include "../container/hash_map.h"

#include "texture.h"

namespace llt
{
	class VulkanBackend;
	class Texture;
	class TextureSampler;
	class Image;

	class TextureMgr
	{
	public:
		TextureMgr();
		~TextureMgr();

		Texture* getTexture(const String& name);
		TextureSampler* getSampler(const String& name, const TextureSampler::Style& style);

		Texture* createFromImage(const String& name,const Image& image);
		Texture* createFromData(const String& name,uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, const byte* data, uint64_t size);
		Texture* createAttachment(const String& name,uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling);
		Texture* createCubeMap(const String& name,VkFormat format, const Image& right, const Image& left, const Image& top, const Image& bottom, const Image& front, const Image& back);

	private:
		HashMap<String, Texture*> m_textureCache;
		HashMap<String, TextureSampler*> m_samplerCache;
	};

	extern TextureMgr* g_textureManager;
}

#endif // VK_TEXTURE_MGR_H_
